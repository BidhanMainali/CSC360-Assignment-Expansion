#include "sfs_common.h"


static void print_entry(const DirEntry *e) {

    char type = 'F';

    if ((e->status & STATUS_DIR) != 0) {
        type = 'D';
    }

    printf("%c %10u %30s %04u/%02u/%02u %02u:%02u:%02u\n",
        type, e->file_size, e->name,
        e->ctime.year, e->ctime.month, e->ctime.day,
        e->ctime.hour, e->ctime.minute, e->ctime.second);
}


static int list_dir(FILE *img, const Superblock *sb, const uint32_t *fat,
    uint32_t dir_start) {

    uint32_t *blocks = malloc(sb->block_count * sizeof(uint32_t));
    uint8_t *buf = malloc(sb->block_size);
    int rc = -1;

    if (blocks != NULL && buf != NULL) {

        uint32_t nblocks = 0;
        int epb = sb->block_size / DIR_ENTRY_SIZE;
        uint32_t b;
        int ok = 1;

        follow_chain(fat, dir_start, sb->block_count, blocks, &nblocks);

        for (b = 0; b < nblocks && ok == 1; b++) {

            if (read_block(img, sb, blocks[b], buf) != 0) {
                ok = 0;
            } else {

                int s;

                for (s = 0; s < epb; s++) {

                    DirEntry de;

                    parse_dir_entry(buf + s * DIR_ENTRY_SIZE, &de);

                    if ((de.status & STATUS_IN_USE) != 0) {
                        print_entry(&de);
                    }
                }
            }
        }

        if (ok == 1) {
            rc = 0;
        }
    }

    free(buf);
    free(blocks);

    return rc;
}


int main(int argc, char *argv[]) {

    FILE *img;
    Superblock sb;
    uint32_t *fat;
    char comps[MAX_PATH_COMPONENTS][NAME_FIELD];
    int ncomp;
    uint32_t dir_start;
    int rc = EXIT_FAILURE;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image> <path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (fs_open(argv[1], "rb", &img, &sb, &fat) != 0) {
        fprintf(stderr, "%s: cannot open or parse image '%s'\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    ncomp = split_path(argv[2], comps, MAX_PATH_COMPONENTS);

    if (resolve_dir(img, &sb, fat, comps, ncomp, &dir_start) == 0) {
        fprintf(stderr, "%s: directory not found: %s\n", argv[0], argv[2]);
    } else if (list_dir(img, &sb, fat, dir_start) == 0) {
        rc = EXIT_SUCCESS;
    }

    free(fat);
    fclose(img);

    return rc;
}

