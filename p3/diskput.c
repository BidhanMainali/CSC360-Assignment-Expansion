#include "sfs_common.h"


static uint8_t *read_host_file(const char *path, uint32_t *size) {

    FILE *f = fopen(path, "rb");
    uint8_t *data = NULL;

    if (f != NULL) {

        long len;
        size_t want;

        fseek(f, 0, SEEK_END);
        len = ftell(f);
        rewind(f);

        if (len > 0) {
            want = (size_t)len;
        } else {
            want = 1;
        }

        if (len >= 0) {

            data = malloc(want);

            if (data != NULL) {

                if (fread(data, 1, (size_t)len, f) != (size_t)len) {
                    free(data);
                    data = NULL;
                } else {
                    *size = (uint32_t)len;
                }
            }
        }

        fclose(f);
    }

    return data;
}


static int create_dir(FILE *img, const Superblock *sb, uint32_t *fat,
    uint32_t parent_start, const char *name, uint32_t *out_start) {

    uint32_t blk;
    int rc = -1;

    if (find_free_blocks(fat, sb->block_count, 1, &blk) == 1) {

        uint8_t *zero = malloc(sb->block_size);
        int ok = (zero != NULL);

        fat[blk] = FAT_EOF;

        if (ok == 1) {

            memset(zero, 0, sb->block_size);

            if (write_block(img, sb, blk, zero) != 0) {
                ok = 0;
            }

            free(zero);
        }

        if (ok == 1 && write_fat_entry(img, sb, blk, FAT_EOF) == 0) {

            DirLoc loc;

            if (dir_get_slot(img, sb, fat, parent_start, &loc) == 1) {

                DirEntry de;

                memset(&de, 0, sizeof(de));

                de.status = STATUS_IN_USE | STATUS_DIR;
                de.start_block = blk;
                de.num_blocks = 1;
                de.file_size = 0;

                fill_current_time(&de.ctime);
                de.mtime = de.ctime;

                strncpy(de.name, name, NAME_FIELD);
                de.name[NAME_FIELD - 1] = '\0';

                if (write_dir_entry(img, sb, loc, &de) == 0) {
                    *out_start = blk;
                    rc = 0;
                }
            }
        }
    }

    return rc;
}


static int resolve_or_create_dir(FILE *img, const Superblock *sb, uint32_t *fat,
    char comps[][NAME_FIELD], int ncomp, uint32_t *out_start) {

    uint32_t current = sb->root_start;
    int ok = 1;
    int i;

    for (i = 0; i < ncomp && ok == 1; i++) {

        DirEntry e;

        if (dir_find(img, sb, fat, current, comps[i], &e) == 1) {

            if ((e.status & STATUS_DIR) != 0) {
                current = e.start_block;
            } else {
                ok = 0;
            }

        } else {

            uint32_t newdir;

            if (create_dir(img, sb, fat, current, comps[i], &newdir) == 0) {
                current = newdir;
            } else {
                ok = 0;
            }
        }
    }

    if (ok == 1) {
        *out_start = current;
    }

    return ok;
}


static int put_file(FILE *img, const Superblock *sb, uint32_t *fat,
    uint32_t dir_start, const char *name, const uint8_t *data, uint32_t size) {

    uint32_t bs = sb->block_size;
    uint32_t need = (size + bs - 1) / bs;
    uint32_t *blocks;
    int rc = -1;

    if (need == 0) {
        need = 1;
    }

    blocks = malloc(need * sizeof(uint32_t));

    if (blocks != NULL &&
        find_free_blocks(fat, sb->block_count, need, blocks) == 1) {

        uint8_t *buf = malloc(bs);
        uint32_t remaining = size;
        uint32_t off = 0;
        uint32_t k;
        int ok = (buf != NULL);

        for (k = 0; k < need; k++) {

            uint32_t next = FAT_EOF;

            if (k + 1 < need) {
                next = blocks[k + 1];
            }

            fat[blocks[k]] = next;
            write_fat_entry(img, sb, blocks[k], next);
        }

        for (k = 0; k < need && ok == 1; k++) {

            uint32_t chunk = bs;

            if (remaining < chunk) {
                chunk = remaining;
            }

            memset(buf, 0, bs);

            if (chunk > 0) {
                memcpy(buf, data + off, chunk);
            }

            if (write_block(img, sb, blocks[k], buf) != 0) {
                ok = 0;
            } else {
                off += chunk;
                remaining -= chunk;
            }
        }

        free(buf);

        if (ok == 1) {

            DirLoc loc;

            if (dir_get_slot(img, sb, fat, dir_start, &loc) == 1) {

                DirEntry de;

                memset(&de, 0, sizeof(de));

                de.status = STATUS_IN_USE | STATUS_FILE;
                de.start_block = blocks[0];
                de.num_blocks = need;
                de.file_size = size;

                fill_current_time(&de.ctime);
                de.mtime = de.ctime;

                strncpy(de.name, name, NAME_FIELD);
                de.name[NAME_FIELD - 1] = '\0';

                if (write_dir_entry(img, sb, loc, &de) == 0) {
                    rc = 0;
                }
            }
        }
    }

    free(blocks);

    return rc;
}


int main(int argc, char *argv[]) {

    FILE *img;
    Superblock sb;
    uint32_t *fat;
    uint8_t *data;
    uint32_t fsize = 0;
    char comps[MAX_PATH_COMPONENTS][NAME_FIELD];
    char filename[NAME_FIELD];
    int ncomp;
    int rc = EXIT_FAILURE;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <image> <src> <dest-path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    data = read_host_file(argv[2], &fsize);

    if (data == NULL) {
        printf("Source file %s not found.\n", argv[2]);
        return EXIT_FAILURE;
    }

    if (fs_open(argv[1], "r+b", &img, &sb, &fat) != 0) {
        fprintf(stderr, "%s: cannot open or parse image '%s'\n", argv[0], argv[1]);
        free(data);
        return EXIT_FAILURE;
    }

    ncomp = split_path(argv[3], comps, MAX_PATH_COMPONENTS);

    if (ncomp == 0) {

        fprintf(stderr, "%s: no destination file given\n", argv[0]);

    } else {

        uint32_t dir_start;

        strncpy(filename, comps[ncomp - 1], NAME_FIELD);
        filename[NAME_FIELD - 1] = '\0';

        if (resolve_or_create_dir(img, &sb, fat, comps, ncomp - 1, &dir_start) == 0) {
            fprintf(stderr, "%s: could not create target directory\n", argv[0]);
        } else if (put_file(img, &sb, fat, dir_start, filename, data, fsize) == 0) {
            rc = EXIT_SUCCESS;
        } else {
            fprintf(stderr, "%s: could not write file (disk full?)\n", argv[0]);
        }
    }

    free(fat);
    free(data);
    fclose(img);

    return rc;
}

