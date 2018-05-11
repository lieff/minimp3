#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define WROOT  5         /* fixed width (bits) of root table (MUST also be changed in the decoder C code) */
#define WMAX   7         /* max width of sub-table */
#define LAMBDA 512       /* Lagrange multiplier for MIPS/bytes trade-off: 1 table jump cost == LAMBDA bytes. Table cost = # of bytes + LAMBDA * # of jumps */
#define OPTIMIZE_SIZE 0  /* flag to force LAMBDA==0 (free jumps, table cost == table size) */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct huff_t
{
    struct huff_t *link;
    struct huff_t *parent;
    struct huff_t *child[2];
    int level;
    unsigned symbol;
    unsigned code;

    struct
    {
        int best_width, size;
        float mips;
    } dp;

    int backref;
    int offset;

    enum { E_PAIR, E_QUAD } symbol_kind;
} huff_t;

static int is_leaf(const huff_t *h)
{
    return !h->child[0] && !h->child[1];
}

static int imax(int a, int b)
{
    return MAX(a, b);
}

static char *BIT_str(unsigned x, int bits)
{   // Print x as bitpattern: 01 (LBS bits)
    static int idx;
    static char buf[8][33]; // up to 8 simultaneous numbers
    char *p = buf[++idx & 7];
    while (bits--)
    {
        *p++ = "01"[(x >> bits) & 1];
    }
    *p++ = 0;
    return buf[idx & 7];
}

huff_t *huff_create(huff_t *h, unsigned symbol, unsigned code, int code_len, int level, int symbol_kind)
{
    if (!h)
    {
        h = (huff_t*)calloc(1, sizeof(huff_t));
        h->level = level;
        h->symbol = code_len ? ~0 : symbol;
        h->code = code_len ? ~0 : code;
        h->dp.size = 1;
        h->dp.mips = 1.f/(1 << level);
        h->backref = -1;
        h->symbol_kind = symbol_kind;
    }
    if (h && code_len > 0)
    {
        int bit = (code >> --code_len) & 1;
        h->child[bit] = huff_create(h->child[bit], symbol, code, code_len, level + 1, symbol_kind);
    }
    return h;
}

void huff_free(huff_t *h)
{
    if (h)
    {
        huff_free(h->child[0]);
        huff_free(h->child[1]);
        free(h);
    }
}

huff_t *read_codebook(int book)
{
    FILE *f = fopen("HUFFCODE", "rt");
    huff_t *tree = NULL;
    int tab = -1, mx, my, lin;
    do
    {
        if (4 != fscanf(f, "\n.table %d %d %d %d", &tab, &mx, &my, &lin))
        {
            fscanf(f, "%*[^\n]");
        }
    } while(tab != book && !feof(f));
    if (tab == book)
    {
        int i, j;
        for (i = 0; i < mx*my; i++)
        {
            int x, y, len, icode = 0;
            char code[30];
            if (book < 32)
            {
                while (4 != fscanf(f, "\n%d %d %d %s", &x, &y, &len, code))
                {
                    fscanf(f, "%*[^\n]");
                }
            } else
            {
                x = 0;
                while (3 != fscanf(f, "\n%d %d %s", &y, &len, code))
                {
                    fscanf(f, "%*[^\n]");
                }
            }
            for (j = 0; j < len; j++)
            {
                icode <<= 1;
                icode |= code[j] - '0';
            }

            tree = huff_create(tree, (x << 16) + y, icode, len, 0, book < 32 ? E_PAIR : E_QUAD);
        }
    }
    fclose(f);
    return tree;
}

int huff_symbols_count(const huff_t *h)
{
    return h ? is_leaf(h) + huff_symbols_count(h->child[0]) + huff_symbols_count(h->child[1]) : 0;
}

int huff_depth(const huff_t *h)
{
    return is_leaf(h) ? 0 : (1 + imax(huff_depth(h->child[0]), huff_depth(h->child[1])));
}

int huff_size(const huff_t *h, int depth)
{
    return is_leaf(h) ? 0 : (--depth < 0) ? h->dp.size : huff_size(h->child[0], depth) + huff_size(h->child[1], depth);
}

float huff_cost(const huff_t *h, int depth)
{
    return is_leaf(h) ? 0 : (--depth < 0) ? h->dp.mips : huff_cost(h->child[0], depth) + huff_cost(h->child[1], depth);
}

const huff_t *huff_decode(const huff_t *h, unsigned code, int code_len)
{
    return (!code_len || is_leaf(h)) ? h : huff_decode(h->child[(code >> (code_len - 1)) & 1], code, code_len - 1);
}

void huff_optimize_partition(huff_t *h)
{
    int i, depth, wmin, wmax;
    if (h && !is_leaf(h))
    {
        // DP call:
        huff_optimize_partition(h->child[0]);
        huff_optimize_partition(h->child[1]);
        depth = huff_depth(h);

        wmin = h->level ? 1 : WROOT;
        wmax = wmin > 1 ? wmin : MIN(depth, WMAX);

        if (h->symbol_kind == E_QUAD && !h->level)
        {
            wmax = wmin = 4;
        }

        h->dp.size = huff_size(h, h->dp.best_width = wmin) + (1 << wmin);
        h->dp.mips = huff_cost(h, h->dp.best_width = wmin) + 1.f/(1 << h->level);

        for (i = wmin + 1; i <= wmax; i++)
        {
            int size = huff_size(h, i) + (1 << i);
            float mips = huff_cost(h, i) + 1.f/(1 << h->level);
            float cost_i;
            float cost_have;

            cost_i = mips*LAMBDA + size;
            cost_have = h->dp.mips*LAMBDA + h->dp.size;
#if OPTIMIZE_SIZE
            cost_i = (float)size;
            cost_have = (float)h->dp.size;
#endif

            if (cost_i < cost_have)
            {
                h->dp.best_width = i;
                h->dp.size = size;
                h->dp.mips = mips;
            }
        }
    }
}

void huff_print_one(const huff_t *h, FILE *f, int parent_level, int last)
{
    if (h->symbol_kind == E_PAIR)
    {
        if (is_leaf(h))
        {
            int x = h->symbol << 16 >> 16;
            int y = h->symbol <<  0 >> 16;
            fprintf(f, last ? "%d" : "%d,", ((h->level - parent_level)*256 + (x << 4) + (y << 0)));
        } else
        {
            assert(h->offset < (1 << 13));
            assert(h->offset);
            fprintf(f, last ? "%d" : "%d,", (-h->offset << 3) | h->dp.best_width);
        }
    } else
    {
        if (is_leaf(h))
        {
            fprintf(f, last ? "%d" : "%d,", (h->symbol*16 + 8 + h->level));
        } else
        {
            fprintf(f, last ? "%d" : "%d,", (h->offset << 3) | h->dp.best_width);
        }
    }
}

void huff_set_links_bfs(huff_t *h, FILE *f)
{
    int print_flag;
    for (print_flag = 0; print_flag <= 1; print_flag++)
    {
        huff_t *q = h;
        huff_t *queue_head = NULL;
        huff_t **queue_tail = &queue_head;
        int offs = 0;
        while (q)
        {
            int i, w = 1 << q->dp.best_width;
            for (i = 0; i < w; i++)
            {
                huff_t *r = (huff_t *)huff_decode(q, i, q->dp.best_width);
                if (print_flag)
                {
                    huff_print_one(r, f, q->level, 0);
                }

                if (!is_leaf(r))
                {
                    r->backref = offs;// + i;
                    *queue_tail = r;
                    queue_tail = &r->link;
                }
            }
            offs += w;
            q = queue_head;
            if (q)
            {
                if ((queue_head = q->link) != NULL)
                {
                    queue_tail = &queue_head;
                }
                q->offset = offs;// - q->backref;
            }
        }
    }
}

void huff_set_links_dfs_recursion(huff_t *h, FILE *f, int print_flag, int *off)
{
    int i, w = 1 << h->dp.best_width;
    h->offset = *off;
    *off += w;
    for (i = 0; print_flag && i < w; i++)
    {
        huff_print_one(huff_decode(h, i, h->dp.best_width), f, h->level, (i + 1) == w);
    }
    for (i = 0; i < w; i++)
    {
        huff_t *q = (huff_t *)huff_decode(h, i, h->dp.best_width);
        if (!is_leaf(q))
        {
            if (print_flag)
                fprintf(f, ",");
            huff_set_links_dfs_recursion(q, f, print_flag, off);
        }
    }
}

void huff_set_links_dfs(huff_t *h, FILE *f)
{
    int off = 0;
    huff_set_links_dfs_recursion(h, f, 0, &off);
    huff_set_links_dfs_recursion(h, f, 1, &off);
};

int main()
{
    int i;
    const int tabn[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,24,32,33 };
    int total_size = 0;
    int entry_bits[32];

    FILE *dst_file = fopen("Huffman_tree.inl", "wt");

    for (i = 0; i < sizeof(tabn)/sizeof(tabn[0]); i++)
    {
        huff_t *h = read_codebook(tabn[i]);
        huff_optimize_partition(h);

        printf("\ntable %2d ", tabn[i]);
        printf("%3d symbols ", huff_symbols_count(h));
        printf("%3d items ", h ? h->dp.size : 0);
        printf("%1d entry ", h ? h->dp.best_width : 0);
        printf("%f average memory reads ", h ? h->dp.mips : 0);
        total_size += h ? h->dp.size : 0;

        fprintf(dst_file, "    static const %s tab%d[] = { ", tabn[i] < 32 ? "int16_t" : "uint8_t", tabn[i]);
        if (h)
        {
            //huff_set_links_bfs(h, dst_file);
            huff_set_links_dfs(h, dst_file);
            entry_bits[i] = h->dp.best_width;
        } else
        {
            fprintf(dst_file, "0");
            entry_bits[i] = 0;
        }
        fprintf(dst_file, " };\n");
        huff_free(h);
    }
#if WROOT > 1
    fprintf(dst_file, "#define HUFF_ENTRY_BITS %d\n", WROOT);
#else
    fprintf(dst_file, "#define HUFF_ENTRY_BITS 0\n");
    fprintf(dst_file, "    static const uint8_t g_entry_bits[] = { ");
    for (i = 0; i < sizeof(tabn)/sizeof(tabn[0]); i++)
    {
        fprintf(dst_file, "%d,", entry_bits[i]);
    }
    fprintf(dst_file, " };\n", i);
#endif
    fclose(dst_file);
    printf("\n// Total: %d items\n", total_size);
}
