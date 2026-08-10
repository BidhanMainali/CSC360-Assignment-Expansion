#ifndef SFS_COMMON_H
#define SFS_COMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <time.h>


#define DIR_ENTRY_SIZE 64
#define NAME_FIELD 31

#define FAT_FREE 0x00000000u
#define FAT_RESERVED 0x00000001u
#define FAT_EOF 0xFFFFFFFFu

#define STATUS_IN_USE 0x01
#define STATUS_FILE 0x02
#define STATUS_DIR 0x04

#define MAX_PATH_COMPONENTS 32


typedef struct FsTime {

    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

} FsTime;


typedef struct Superblock {

    uint16_t block_size;
    uint32_t block_count;
    uint32_t fat_start;
    uint32_t fat_blocks;
    uint32_t root_start;
    uint32_t root_blocks;

} Superblock;


typedef struct DirEntry {

    uint8_t status;
    uint32_t start_block;
    uint32_t num_blocks;
    uint32_t file_size;
    FsTime ctime;
    FsTime mtime;
    char name[NAME_FIELD];

} DirEntry;


typedef struct DirLoc {

    uint32_t block;
    int index;

} DirLoc;


int fs_open(const char *path, const char *mode, FILE **img,
    Superblock *sb, uint32_t **fat);


int read_superblock(FILE *img, Superblock *sb);

uint32_t *read_fat(FILE *img, const Superblock *sb);

int read_block(FILE *img, const Superblock *sb, uint32_t block, uint8_t *buf);

int write_block(FILE *img, const Superblock *sb, uint32_t block,
    const uint8_t *buf);

int write_fat_entry(FILE *img, const Superblock *sb, uint32_t block,
    uint32_t value);


void follow_chain(const uint32_t *fat, uint32_t start, uint32_t cap,
    uint32_t *out_blocks, uint32_t *out_count);

int find_free_blocks(const uint32_t *fat, uint32_t block_count,
    uint32_t need, uint32_t *out_blocks);


void parse_dir_entry(const uint8_t *buf, DirEntry *e);

void encode_dir_entry(const DirEntry *e, uint8_t *buf);


int split_path(const char *path, char comps[][NAME_FIELD], int max);

void build_path(char comps[][NAME_FIELD], int n, char *out);


int dir_find(FILE *img, const Superblock *sb, const uint32_t *fat,
    uint32_t dir_start, const char *name, DirEntry *out);

int resolve_dir(FILE *img, const Superblock *sb, const uint32_t *fat,
    char comps[][NAME_FIELD], int ncomp, uint32_t *out_start);


int dir_find_free_slot(FILE *img, const Superblock *sb, const uint32_t *fat,
    uint32_t dir_start, DirLoc *out);

int dir_get_slot(FILE *img, const Superblock *sb, uint32_t *fat,
    uint32_t dir_start, DirLoc *out);

int write_dir_entry(FILE *img, const Superblock *sb, DirLoc loc,
    const DirEntry *e);


void fill_current_time(FsTime *t);


#endif
