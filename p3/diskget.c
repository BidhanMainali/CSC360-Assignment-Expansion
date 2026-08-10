#include "sfs_common.h"


static int extract_file(FILE *img, const Superblock *sb, const uint32_t *fat,
    const DirEntry *e, const char *out_path) {

    FILE *out = fopen(out_path, "wb");
    uint32_t *blocks;
    uint8_t *buf;
    int rc = -1;

    if (out == NULL) {
        perror("fopen");
        return -1;
    }

    blocks = malloc(sb->block_count * sizeof(uint32_t));
    buf = malloc(sb->block_size);

    if (blocks != NULL && buf != NULL) {

        uint32_t nblocks = 0;
        uint32_t remaining = e->file_size;
        uint32_t b;
        int ok = 1;

        follow_chain(fat, e->start_block, sb->block_count, blocks, &nblocks);

        for (b = 0; b < nblocks && ok == 1; b++) {

            if (read_block(img, sb, blocks[b], buf) != 0) {
                ok = 0;
            } else {

                uint32_t chunk = sb->block_size;

                if (remaining < chunk) {
                    chunk = remaining;
                }

                if (fwrite(buf, 1, chunk, out) != chunk) {
                    ok = 0;
                } else {
                    remaining -= chunk;
                }
            }
        }

        if (ok == 1) {
            rc = 0;
        }
    }

    free(buf);
    free(blocks);
    fclose(out);

    return rc;
}


int main(int argc, char *argv[]) {

    FILE *img;
    Superblock sb;
    uint32_t *fat;
    char comps[MAX_PATH_COMPONENTS][NAME_FIELD];
    char parent[MAX_PATH_COMPONENTS * NAME_FIELD];
    char filename[NAME_FIELD];
    int ncomp;
    int rc = EXIT_FAILURE;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <image> <src-path> <dest>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (fs_open(argv[1], "rb", &img, &sb, &fat) != 0) {
        fprintf(stderr, "%s: cannot open or parse image '%s'\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    ncomp = split_path(argv[2], comps, MAX_PATH_COMPONENTS);

    if (ncomp == 0) {

        fprintf(stderr, "%s: no source file given\n", argv[0]);

    } else {

        DirEntry e;
        uint32_t dir_start;
        int found = 0;

        strncpy(filename, comps[ncomp - 1], NAME_FIELD);
        filename[NAME_FIELD - 1] = '\0';

        build_path(comps, ncomp - 1, parent);

        if (resolve_dir(img, &sb, fat, comps, ncomp - 1, &dir_start) == 1 &&
            dir_find(img, &sb, fat, dir_start, filename, &e) == 1 &&
            (e.status & STATUS_FILE) != 0) {
            found = 1;
        }

        if (found == 1) {

            if (extract_file(img, &sb, fat, &e, argv[3]) == 0) {
                rc = EXIT_SUCCESS;
            } else {
                fprintf(stderr, "%s: failed to copy %s\n", argv[0], filename);
            }

        } else {
            printf("Requested file %s not found in %s.\n", filename, parent);
        }
    }

    free(fat);
    fclose(img);

    return rc;
}

