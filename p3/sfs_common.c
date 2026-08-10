
#include "sfs_common.h"


int fs_open(const char *path, const char *mode, FILE **img,
    Superblock *sb, uint32_t **fat) {

    int rc = -1;

    *img = fopen(path, mode);

    if (*img != NULL) {

        if (read_superblock(*img, sb) == 0) {

            *fat = read_fat(*img, sb);

            if (*fat != NULL) {
                rc = 0;
            }
        }

        if (rc != 0) {
            fclose(*img);
            *img = NULL;
        }
    }

    return rc;
}


int read_superblock(FILE *img, Superblock *sb) {

    uint8_t buf[512];
    uint16_t v16;
    uint32_t v32;
    int rc = -1;

    if (fseek(img, 0, SEEK_SET) == 0 && fread(buf, 1, 512, img) == 512) {

        memcpy(&v16, buf + 8, 2);
        sb->block_size = ntohs(v16);

        memcpy(&v32, buf + 10, 4);
        sb->block_count = ntohl(v32);

        memcpy(&v32, buf + 14, 4);
        sb->fat_start = ntohl(v32);

        memcpy(&v32, buf + 18, 4);
        sb->fat_blocks = ntohl(v32);

        memcpy(&v32, buf + 22, 4);
        sb->root_start = ntohl(v32);

        memcpy(&v32, buf + 26, 4);
        sb->root_blocks = ntohl(v32);

        rc = 0;
    }

    return rc;
}


uint32_t *read_fat(FILE *img, const Superblock *sb) {

    uint32_t count = sb->block_count;
    uint32_t *fat = malloc(count * sizeof(uint32_t));
    long offset = (long)sb->fat_start * sb->block_size;
    int ok = 0;

    if (fat != NULL && fseek(img, offset, SEEK_SET) == 0) {

        uint32_t i;
        uint32_t raw;

        ok = 1;

        for (i = 0; i < count && ok == 1; i++) {

            if (fread(&raw, 4, 1, img) != 1) {
                ok = 0;
            } else {
                fat[i] = ntohl(raw);
            }
        }
    }

    if (ok == 0 && fat != NULL) {
        free(fat);
        fat = NULL;
    }

    return fat;
}


int read_block(FILE *img, const Superblock *sb, uint32_t block, uint8_t *buf) {

    long offset = (long)block * sb->block_size;
    int rc = -1;

    if (fseek(img, offset, SEEK_SET) == 0 &&
        fread(buf, 1, sb->block_size, img) == sb->block_size) {
        rc = 0;
    }

    return rc;
}


int write_block(FILE *img, const Superblock *sb, uint32_t block,
    const uint8_t *buf) {

    long offset = (long)block * sb->block_size;
    int rc = -1;

    if (fseek(img, offset, SEEK_SET) == 0 &&
        fwrite(buf, 1, sb->block_size, img) == sb->block_size) {
        rc = 0;
    }

    return rc;
}


int write_fat_entry(FILE *img, const Superblock *sb, uint32_t block,
    uint32_t value) {

    long offset = (long)sb->fat_start * sb->block_size + (long)block * 4;
    uint32_t raw = htonl(value);
    int rc = -1;

    if (fseek(img, offset, SEEK_SET) == 0 && fwrite(&raw, 4, 1, img) == 1) {
        rc = 0;
    }

    return rc;
}


void follow_chain(const uint32_t *fat, uint32_t start, uint32_t cap,
    uint32_t *out_blocks, uint32_t *out_count) {

    uint32_t current = start;
    uint32_t count = 0;

    while (current != FAT_EOF && count < cap) {

        out_blocks[count] = current;
        count++;
        current = fat[current];
    }

    *out_count = count;
}


int find_free_blocks(const uint32_t *fat, uint32_t block_count,
    uint32_t need, uint32_t *out_blocks) {

    uint32_t got = 0;
    uint32_t i;

    for (i = 0; i < block_count && got < need; i++) {

        if (fat[i] == FAT_FREE) {
            out_blocks[got] = i;
            got++;
        }
    }

    return (got == need);
}


void parse_dir_entry(const uint8_t *buf, DirEntry *e) {

    uint16_t v16;
    uint32_t v32;

    e->status = buf[0];

    memcpy(&v32, buf + 1, 4);
    e->start_block = ntohl(v32);

    memcpy(&v32, buf + 5, 4);
    e->num_blocks = ntohl(v32);

    memcpy(&v32, buf + 9, 4);
    e->file_size = ntohl(v32);

    memcpy(&v16, buf + 13, 2);
    e->ctime.year = ntohs(v16);
    e->ctime.month = buf[15];
    e->ctime.day = buf[16];
    e->ctime.hour = buf[17];
    e->ctime.minute = buf[18];
    e->ctime.second = buf[19];

    memcpy(&v16, buf + 20, 2);
    e->mtime.year = ntohs(v16);
    e->mtime.month = buf[22];
    e->mtime.day = buf[23];
    e->mtime.hour = buf[24];
    e->mtime.minute = buf[25];
    e->mtime.second = buf[26];

    memcpy(e->name, buf + 27, NAME_FIELD);
    e->name[NAME_FIELD - 1] = '\0';
}


void encode_dir_entry(const DirEntry *e, uint8_t *buf) {

    uint16_t v16;
    uint32_t v32;
    size_t nlen;

    memset(buf, 0, DIR_ENTRY_SIZE);

    buf[0] = e->status;

    v32 = htonl(e->start_block);
    memcpy(buf + 1, &v32, 4);

    v32 = htonl(e->num_blocks);
    memcpy(buf + 5, &v32, 4);

    v32 = htonl(e->file_size);
    memcpy(buf + 9, &v32, 4);

    v16 = htons(e->ctime.year);
    memcpy(buf + 13, &v16, 2);
    buf[15] = e->ctime.month;
    buf[16] = e->ctime.day;
    buf[17] = e->ctime.hour;
    buf[18] = e->ctime.minute;
    buf[19] = e->ctime.second;

    v16 = htons(e->mtime.year);
    memcpy(buf + 20, &v16, 2);
    buf[22] = e->mtime.month;
    buf[23] = e->mtime.day;
    buf[24] = e->mtime.hour;
    buf[25] = e->mtime.minute;
    buf[26] = e->mtime.second;

    nlen = strlen(e->name);

    if (nlen > NAME_FIELD - 1) {
        nlen = NAME_FIELD - 1;
    }

    memcpy(buf + 27, e->name, nlen);

    memset(buf + 58, 0xFF, 6);
}


int split_path(const char *path, char comps[][NAME_FIELD], int max) {

    int count = 0;
    int len = 0;
    int i = 0;

    while (path[i] != '\0' && count < max) {

        if (path[i] == '/') {

            if (len > 0) {
                comps[count][len] = '\0';
                count++;
                len = 0;
            }

        } else {

            if (len < NAME_FIELD - 1) {
                comps[count][len] = path[i];
                len++;
            }
        }

        i++;
    }

    if (len > 0 && count < max) {
        comps[count][len] = '\0';
        count++;
    }

    return count;
}


void build_path(char comps[][NAME_FIELD], int n, char *out) {

    if (n <= 0) {

        out[0] = '/';
        out[1] = '\0';

    } else {

        int pos = 0;
        int i;

        for (i = 0; i < n; i++) {

            int j = 0;

            out[pos] = '/';
            pos++;

            while (comps[i][j] != '\0') {
                out[pos] = comps[i][j];
                pos++;
                j++;
            }
        }

        out[pos] = '\0';
    }
}


int dir_find(FILE *img, const Superblock *sb, const uint32_t *fat,
    uint32_t dir_start, const char *name, DirEntry *out) {

    uint32_t *blocks = malloc(sb->block_count * sizeof(uint32_t));
    uint8_t *buf = malloc(sb->block_size);
    int found = 0;

    if (blocks != NULL && buf != NULL) {

        uint32_t nblocks = 0;
        int epb = sb->block_size / DIR_ENTRY_SIZE;
        uint32_t b;

        follow_chain(fat, dir_start, sb->block_count, blocks, &nblocks);

        for (b = 0; b < nblocks && found == 0; b++) {

            if (read_block(img, sb, blocks[b], buf) == 0) {

                int s;

                for (s = 0; s < epb && found == 0; s++) {

                    DirEntry de;

                    parse_dir_entry(buf + s * DIR_ENTRY_SIZE, &de);

                    if ((de.status & STATUS_IN_USE) != 0 &&
                        strcmp(de.name, name) == 0) {
                        *out = de;
                        found = 1;
                    }
                }
            }
        }
    }

    free(buf);
    free(blocks);

    return found;
}


int resolve_dir(FILE *img, const Superblock *sb, const uint32_t *fat,
    char comps[][NAME_FIELD], int ncomp, uint32_t *out_start) {

    uint32_t current = sb->root_start;
    int ok = 1;
    int i;

    for (i = 0; i < ncomp && ok == 1; i++) {

        DirEntry e;

        if (dir_find(img, sb, fat, current, comps[i], &e) == 1 &&
            (e.status & STATUS_DIR) != 0) {
            current = e.start_block;
        } else {
            ok = 0;
        }
    }

    if (ok == 1) {
        *out_start = current;
    }

    return ok;
}


int dir_find_free_slot(FILE *img, const Superblock *sb, const uint32_t *fat,
    uint32_t dir_start, DirLoc *out) {

    uint32_t *blocks = malloc(sb->block_count * sizeof(uint32_t));
    uint8_t *buf = malloc(sb->block_size);
    int found = 0;

    if (blocks != NULL && buf != NULL) {

        uint32_t nblocks = 0;
        int epb = sb->block_size / DIR_ENTRY_SIZE;
        uint32_t b;

        follow_chain(fat, dir_start, sb->block_count, blocks, &nblocks);

        for (b = 0; b < nblocks && found == 0; b++) {

            if (read_block(img, sb, blocks[b], buf) == 0) {

                int s;

                for (s = 0; s < epb && found == 0; s++) {

                    DirEntry de;

                    parse_dir_entry(buf + s * DIR_ENTRY_SIZE, &de);

                    if ((de.status & STATUS_IN_USE) == 0) {
                        out->block = blocks[b];
                        out->index = s;
                        found = 1;
                    }
                }
            }
        }
    }

    free(buf);
    free(blocks);

    return found;
}


int dir_get_slot(FILE *img, const Superblock *sb, uint32_t *fat,
    uint32_t dir_start, DirLoc *out) {

    int found = dir_find_free_slot(img, sb, fat, dir_start, out);

    if (found == 0) {

        uint32_t newblk;

        if (find_free_blocks(fat, sb->block_count, 1, &newblk) == 1) {

            uint32_t last = dir_start;
            uint8_t *zero = malloc(sb->block_size);

            while (fat[last] != FAT_EOF) {
                last = fat[last];
            }

            fat[last] = newblk;
            fat[newblk] = FAT_EOF;

            write_fat_entry(img, sb, last, newblk);
            write_fat_entry(img, sb, newblk, FAT_EOF);

            if (zero != NULL) {
                memset(zero, 0, sb->block_size);
                write_block(img, sb, newblk, zero);
                free(zero);
            }

            out->block = newblk;
            out->index = 0;
            found = 1;
        }
    }

    return found;
}


int write_dir_entry(FILE *img, const Superblock *sb, DirLoc loc,
    const DirEntry *e) {

    uint8_t buf[DIR_ENTRY_SIZE];
    long offset = (long)loc.block * sb->block_size +
        (long)loc.index * DIR_ENTRY_SIZE;
    int rc = -1;

    encode_dir_entry(e, buf);

    if (fseek(img, offset, SEEK_SET) == 0 &&
        fwrite(buf, 1, DIR_ENTRY_SIZE, img) == DIR_ENTRY_SIZE) {
        rc = 0;
    }

    return rc;
}


void fill_current_time(FsTime *t) {

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);

    t->year = (uint16_t)(lt->tm_year + 1900);
    t->month = (uint8_t)(lt->tm_mon + 1);
    t->day = (uint8_t)lt->tm_mday;
    t->hour = (uint8_t)lt->tm_hour;
    t->minute = (uint8_t)lt->tm_min;
    t->second = (uint8_t)lt->tm_sec;
}

