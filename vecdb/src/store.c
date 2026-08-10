#include "store.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

/* 8-byte magic identifying a vecdb file. */
static const uint8_t VDB_MAGIC[VDB_MAGIC_LEN] = {
    'V', 'E', 'C', 'D', 'B', 'F', 'S', '1'
};

/* Byte offsets of each field within the 512-byte header block. */
enum {
    OFF_MAGIC       = 0,   /* 8 bytes */
    OFF_VERSION     = 8,   /* u32     */
    OFF_HEADER_SIZE = 12,  /* u16     */
    OFF_METRIC      = 14,  /* u16     */
    OFF_INDEX_TYPE  = 16,  /* u16     */
    OFF_RESERVED0   = 18,  /* u16     */
    OFF_DIM         = 20,  /* u32     */
    OFF_COUNT       = 24,  /* u32     */
    OFF_HASH_SEED   = 28,  /* u32     */
    OFF_VECTORS_OFF = 32,  /* u64     */
    OFF_VECTORS_SZ  = 40,  /* u64     */
    OFF_PAYLOAD_OFF = 48,  /* u64     */
    OFF_PAYLOAD_SZ  = 56,  /* u64     */
    OFF_IDMAP_OFF   = 64,  /* u64     */
    OFF_IDMAP_SZ    = 72,  /* u64     */
    OFF_IDF_OFF     = 80,  /* u64     */
    OFF_IDF_SZ      = 88   /* u64     */
};

/* Serialize `hdr` into a zeroed 512-byte block. */
static void encode_header(const VdbHeader *hdr, uint8_t *buf) {
    memset(buf, 0, VDB_HEADER_SIZE);

    memcpy(buf + OFF_MAGIC, VDB_MAGIC, VDB_MAGIC_LEN);
    put_be32(buf + OFF_VERSION,     hdr->version);
    put_be16(buf + OFF_HEADER_SIZE, (uint16_t)VDB_HEADER_SIZE);
    put_be16(buf + OFF_METRIC,      hdr->metric);
    put_be16(buf + OFF_INDEX_TYPE,  hdr->index_type);
    put_be32(buf + OFF_DIM,         hdr->dim);
    put_be32(buf + OFF_COUNT,       hdr->count);
    put_be32(buf + OFF_HASH_SEED,   hdr->hash_seed);
    put_be64(buf + OFF_VECTORS_OFF, hdr->vectors_off);
    put_be64(buf + OFF_VECTORS_SZ,  hdr->vectors_size);
    put_be64(buf + OFF_PAYLOAD_OFF, hdr->payload_off);
    put_be64(buf + OFF_PAYLOAD_SZ,  hdr->payload_size);
    put_be64(buf + OFF_IDMAP_OFF,   hdr->idmap_off);
    put_be64(buf + OFF_IDMAP_SZ,    hdr->idmap_size);
    put_be64(buf + OFF_IDF_OFF,     hdr->idf_off);
    put_be64(buf + OFF_IDF_SZ,      hdr->idf_size);
}

/* Parse a 512-byte header block into `hdr`. Returns 0 on success, -1 if the
   magic or header size does not match. */
static int decode_header(const uint8_t *buf, VdbHeader *hdr) {
    if (memcmp(buf + OFF_MAGIC, VDB_MAGIC, VDB_MAGIC_LEN) != 0) {
        return -1;
    }
    if (get_be16(buf + OFF_HEADER_SIZE) != VDB_HEADER_SIZE) {
        return -1;
    }

    hdr->version      = get_be32(buf + OFF_VERSION);
    hdr->metric       = get_be16(buf + OFF_METRIC);
    hdr->index_type   = get_be16(buf + OFF_INDEX_TYPE);
    hdr->dim          = get_be32(buf + OFF_DIM);
    hdr->count        = get_be32(buf + OFF_COUNT);
    hdr->hash_seed    = get_be32(buf + OFF_HASH_SEED);
    hdr->vectors_off  = get_be64(buf + OFF_VECTORS_OFF);
    hdr->vectors_size = get_be64(buf + OFF_VECTORS_SZ);
    hdr->payload_off  = get_be64(buf + OFF_PAYLOAD_OFF);
    hdr->payload_size = get_be64(buf + OFF_PAYLOAD_SZ);
    hdr->idmap_off    = get_be64(buf + OFF_IDMAP_OFF);
    hdr->idmap_size   = get_be64(buf + OFF_IDMAP_SZ);
    hdr->idf_off      = get_be64(buf + OFF_IDF_OFF);
    hdr->idf_size     = get_be64(buf + OFF_IDF_SZ);

    return 0;
}

int vdb_create(const char *path, uint32_t dim, uint32_t hash_seed) {
    uint8_t   buf[VDB_HEADER_SIZE];
    VdbHeader hdr;
    FILE     *fp;

    if (dim == 0) {
        fprintf(stderr, "vecdb: dimension must be greater than 0\n");
        return -1;
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.version    = VDB_VERSION;
    hdr.dim        = dim;
    hdr.count      = 0;
    hdr.hash_seed  = hash_seed;
    hdr.metric     = VDB_METRIC_COSINE;
    hdr.index_type = VDB_INDEX_FLAT;
    /* Region offsets/sizes stay 0: an empty store has no data yet. */

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "vecdb: cannot create '%s'\n", path);
        return -1;
    }

    encode_header(&hdr, buf);

    if (fwrite(buf, 1, VDB_HEADER_SIZE, fp) != VDB_HEADER_SIZE) {
        fprintf(stderr, "vecdb: failed to write header to '%s'\n", path);
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "vecdb: failed to finalize '%s'\n", path);
        return -1;
    }

    return 0;
}

int vdb_open(const char *path, Vdb *db) {
    uint8_t buf[VDB_HEADER_SIZE];
    FILE   *fp;

    memset(db, 0, sizeof(*db));

    fp = fopen(path, "r+b");
    if (fp == NULL) {
        fprintf(stderr, "vecdb: cannot open '%s'\n", path);
        return -1;
    }

    if (fread(buf, 1, VDB_HEADER_SIZE, fp) != VDB_HEADER_SIZE) {
        fprintf(stderr, "vecdb: '%s' is too small to be a vecdb file\n", path);
        fclose(fp);
        return -1;
    }

    if (decode_header(buf, &db->hdr) != 0) {
        fprintf(stderr, "vecdb: '%s' is not a valid vecdb file\n", path);
        fclose(fp);
        return -1;
    }

    if (db->hdr.version != VDB_VERSION) {
        fprintf(stderr, "vecdb: unsupported version %u in '%s'\n",
                db->hdr.version, path);
        fclose(fp);
        return -1;
    }

    db->fp   = fp;
    db->path = strdup(path);
    if (db->path == NULL) {
        die("out of memory");
    }

    return 0;
}

void vdb_close(Vdb *db) {
    if (db == NULL) {
        return;
    }
    if (db->fp != NULL) {
        fclose(db->fp);
        db->fp = NULL;
    }
    free(db->path);
    db->path = NULL;
}

static const char *metric_name(uint16_t metric) {
    switch (metric) {
        case VDB_METRIC_COSINE: return "cosine";
        default:                return "unknown";
    }
}

static const char *index_name(uint16_t index_type) {
    switch (index_type) {
        case VDB_INDEX_FLAT: return "flat";
        default:             return "unknown";
    }
}

void vdb_print_stats(const Vdb *db, FILE *out) {
    long size = -1;

    if (db->fp != NULL && fseek(db->fp, 0, SEEK_END) == 0) {
        size = ftell(db->fp);
    }

    fprintf(out, "Store:        %s\n", db->path ? db->path : "(unknown)");
    fprintf(out, "Version:      %u\n", db->hdr.version);
    fprintf(out, "Dimension:    %u\n", db->hdr.dim);
    fprintf(out, "Vectors:      %u\n", db->hdr.count);
    fprintf(out, "Metric:       %s\n", metric_name(db->hdr.metric));
    fprintf(out, "Index:        %s\n", index_name(db->hdr.index_type));
    fprintf(out, "Hash seed:    %u\n", db->hdr.hash_seed);
    fprintf(out, "Header size:  %u bytes\n", VDB_HEADER_SIZE);
    if (size >= 0) {
        fprintf(out, "File size:    %ld bytes\n", size);
    }
}

void vdb_data_init(VdbData *data, uint32_t dim, uint32_t hash_seed) {
    memset(data, 0, sizeof(*data));
    data->hdr.version    = VDB_VERSION;
    data->hdr.dim        = dim;
    data->hdr.count      = 0;
    data->hdr.hash_seed  = hash_seed;
    data->hdr.metric     = VDB_METRIC_COSINE;
    data->hdr.index_type = VDB_INDEX_FLAT;
}

int vdb_data_add(VdbData *data, const float *vec, const char *text) {
    uint32_t dim = data->hdr.dim;
    char    *copy;

    /* Grow the parallel arrays when full. The vector pointer is committed
       before growing the payload array so the struct is never left holding
       a freed pointer if the second reallocation fails. */
    if (data->count == data->cap) {
        uint32_t newcap = (data->cap == 0) ? 16 : data->cap * 2;
        float   *nv;
        char   **np;

        nv = realloc(data->vectors, (size_t)newcap * dim * sizeof(float));
        if (nv == NULL) {
            return -1;
        }
        data->vectors = nv;

        np = realloc(data->payloads, (size_t)newcap * sizeof(char *));
        if (np == NULL) {
            return -1;
        }
        data->payloads = np;

        data->cap = newcap;
    }

    copy = strdup(text);
    if (copy == NULL) {
        return -1;
    }

    memcpy(data->vectors + (size_t)data->count * dim, vec,
           (size_t)dim * sizeof(float));
    data->payloads[data->count] = copy;
    data->count++;
    data->hdr.count = data->count;

    return 0;
}

void vdb_data_free(VdbData *data) {
    uint32_t i;

    if (data == NULL) {
        return;
    }
    for (i = 0; i < data->count; i++) {
        free(data->payloads[i]);
    }
    free(data->payloads);
    free(data->vectors);
    memset(data, 0, sizeof(*data));
}

int vdb_write(const char *path, VdbData *data) {
    FILE    *fp;
    uint8_t  hdrbuf[VDB_HEADER_SIZE];
    uint32_t dim = data->hdr.dim;
    uint64_t vectors_size = (uint64_t)data->count * dim * sizeof(float);
    uint64_t payload_size = 0;
    uint32_t i;

    for (i = 0; i < data->count; i++) {
        payload_size += 4u + (uint64_t)strlen(data->payloads[i]);
    }

    /* Record the layout in the header before serializing it. */
    data->hdr.count        = data->count;
    data->hdr.vectors_off  = VDB_HEADER_SIZE;
    data->hdr.vectors_size = vectors_size;
    data->hdr.payload_off  = VDB_HEADER_SIZE + vectors_size;
    data->hdr.payload_size = payload_size;
    data->hdr.idmap_off    = 0;
    data->hdr.idmap_size   = 0;
    data->hdr.idf_off      = 0;
    data->hdr.idf_size     = 0;

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "vecdb: cannot write '%s'\n", path);
        return -1;
    }

    encode_header(&data->hdr, hdrbuf);
    if (fwrite(hdrbuf, 1, VDB_HEADER_SIZE, fp) != VDB_HEADER_SIZE) {
        fprintf(stderr, "vecdb: failed to write header to '%s'\n", path);
        fclose(fp);
        return -1;
    }

    /* Vectors: one contiguous block of raw float32 in host byte order. */
    if (vectors_size > 0) {
        size_t nfloats = (size_t)data->count * dim;

        if (fwrite(data->vectors, sizeof(float), nfloats, fp) != nfloats) {
            fprintf(stderr, "vecdb: failed to write vectors to '%s'\n", path);
            fclose(fp);
            return -1;
        }
    }

    /* Payloads: each is a big-endian length followed by its raw bytes. */
    for (i = 0; i < data->count; i++) {
        uint8_t lenbuf[4];
        size_t  len = strlen(data->payloads[i]);

        put_be32(lenbuf, (uint32_t)len);

        if (fwrite(lenbuf, 1, 4, fp) != 4 ||
            (len > 0 && fwrite(data->payloads[i], 1, len, fp) != len)) {
            fprintf(stderr, "vecdb: failed to write payload to '%s'\n", path);
            fclose(fp);
            return -1;
        }
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "vecdb: failed to finalize '%s'\n", path);
        return -1;
    }

    return 0;
}

int vdb_load(const char *path, VdbData *data) {
    FILE    *fp;
    uint8_t  hdrbuf[VDB_HEADER_SIZE];
    uint32_t dim;
    uint32_t count;
    uint32_t i;
    size_t   nfloats;

    memset(data, 0, sizeof(*data));

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "vecdb: cannot open '%s'\n", path);
        return -1;
    }

    if (fread(hdrbuf, 1, VDB_HEADER_SIZE, fp) != VDB_HEADER_SIZE) {
        fprintf(stderr, "vecdb: '%s' is too small to be a vecdb file\n", path);
        fclose(fp);
        return -1;
    }

    if (decode_header(hdrbuf, &data->hdr) != 0) {
        fprintf(stderr, "vecdb: '%s' is not a valid vecdb file\n", path);
        fclose(fp);
        return -1;
    }

    if (data->hdr.version != VDB_VERSION) {
        fprintf(stderr, "vecdb: unsupported version %u in '%s'\n",
                data->hdr.version, path);
        fclose(fp);
        return -1;
    }

    dim   = data->hdr.dim;
    count = data->hdr.count;

    if (count == 0) {
        fclose(fp);
        return 0;   /* empty store: nothing more to read */
    }

    nfloats        = (size_t)count * dim;
    data->vectors  = malloc(nfloats * sizeof(float));
    data->payloads = malloc((size_t)count * sizeof(char *));

    if (data->vectors == NULL || data->payloads == NULL) {
        fprintf(stderr, "vecdb: out of memory loading '%s'\n", path);
        vdb_data_free(data);
        fclose(fp);
        return -1;
    }

    /* Zero the payload slots so vdb_data_free stays safe if a later read
       fails and only some payloads have been allocated. */
    memset(data->payloads, 0, (size_t)count * sizeof(char *));
    data->count = count;
    data->cap   = count;

    if (fread(data->vectors, sizeof(float), nfloats, fp) != nfloats) {
        fprintf(stderr, "vecdb: truncated vectors in '%s'\n", path);
        vdb_data_free(data);
        fclose(fp);
        return -1;
    }

    for (i = 0; i < count; i++) {
        uint8_t  lenbuf[4];
        uint32_t len;
        char    *s;

        if (fread(lenbuf, 1, 4, fp) != 4) {
            fprintf(stderr, "vecdb: truncated payload in '%s'\n", path);
            vdb_data_free(data);
            fclose(fp);
            return -1;
        }
        len = get_be32(lenbuf);

        s = malloc((size_t)len + 1);
        if (s == NULL) {
            fprintf(stderr, "vecdb: out of memory loading '%s'\n", path);
            vdb_data_free(data);
            fclose(fp);
            return -1;
        }

        if (len > 0 && fread(s, 1, len, fp) != len) {
            fprintf(stderr, "vecdb: truncated payload in '%s'\n", path);
            free(s);
            vdb_data_free(data);
            fclose(fp);
            return -1;
        }

        s[len] = '\0';
        data->payloads[i] = s;
    }

    fclose(fp);
    return 0;
}
