#include "sfs_common.h"


static void count_fat(const uint32_t *fat, uint32_t block_count,
    uint32_t *free_c, uint32_t *reserved_c, uint32_t *allocated_c) {

    uint32_t f = 0;
    uint32_t r = 0;
    uint32_t a = 0;
    uint32_t i;

    for (i = 0; i < block_count; i++) {

        if (fat[i] == FAT_FREE) {
            f++;
        } else if (fat[i] == FAT_RESERVED) {
            r++;
        } else {
            a++;
        }
    }

    *free_c = f;
    *reserved_c = r;
    *allocated_c = a;
}


static void print_info(const Superblock *sb, uint32_t free_c,
    uint32_t reserved_c, uint32_t allocated_c) {

    printf("Super block information:\n");
    printf("Block size: %u\n", sb->block_size);
    printf("Block count: %u\n", sb->block_count);
    printf("FAT starts: %u\n", sb->fat_start);
    printf("FAT blocks: %u\n", sb->fat_blocks);
    printf("Root directory start: %u\n", sb->root_start);
    printf("Root directory blocks: %u\n", sb->root_blocks);
    printf("\n");
    printf("FAT information:\n");
    printf("Free Blocks: %u\n", free_c);
    printf("Reserved Blocks: %u\n", reserved_c);
    printf("Allocated Blocks: %u\n", allocated_c);
}


int main(int argc, char *argv[]) {

    FILE *img;
    Superblock sb;
    uint32_t *fat;
    uint32_t free_c, reserved_c, allocated_c;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (fs_open(argv[1], "rb", &img, &sb, &fat) != 0) {
        fprintf(stderr, "%s: cannot open or parse image '%s'\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    count_fat(fat, sb.block_count, &free_c, &reserved_c, &allocated_c);
    print_info(&sb, free_c, reserved_c, allocated_c);

    free(fat);
    fclose(img);

    return EXIT_SUCCESS;
}

