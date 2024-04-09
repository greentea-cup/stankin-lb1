#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

size_t npow2(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    v++;
    return v;
}

// returns 1 on success, 0 on failure
// success if str is in format /^\+?[0-9]+\s*$/
int parse_uint(char *str, size_t *result) {
    return 0;
}

typedef struct {
    size_t id;
    int64_t c1;
    double c2; // todo: find some float64_t
    char c3[16];
    bool c4;
    char c5[32];
} dbrow_t;

typedef struct {
    dbrow_t *rows;
    size_t len;
    size_t cap;
} table_t;

table_t *table_new(size_t cap) {
    size_t cap2 = npow2(cap);
    if (cap2 < cap || cap2 == 0) goto bad_cap;
    table_t *table = malloc(1 * sizeof(table_t));
    if (table == NULL) goto no_table;
    dbrow_t *rows = calloc(cap2, sizeof(dbrow_t));
    if (rows == NULL) goto no_rows;
    table->rows = rows;
    table->len = 0;
    table->cap = cap2;
    return table;
no_rows:
    free(table);
no_table:
bad_cap:
    return NULL;
}

void table_free(table_t *table) {
    free(table->rows);
    free(table);
}

int main(void) {
    FILE *fin = stdin; // no free
    FILE *fout = stdout; // no free
    FILE *ferr = stderr; // no free
    table_t *table = NULL;
    char line[MAX_LINE_SIZE] = {0};
    while (1) {
        if (fgets(line, MAX_LINE_SIZE, fin) == NULL) {
            fprintf(ferr, "Get line error (fgets returned NULL)\n");
            break;
        }
        if (ferror(fin)) {
            fprintf(ferr, "Everything is bad\n");
            break;
        }
        else if (feof(fin)) {
            fprintf(ferr, "EOF\n");
            break;
        }
        else if (strlen(line) == 5 && !strncmp("quit\n", line, 5)) {
            fprintf(fout, "Quitting\n");
            break;
        }
        else if (strlen(line) == 5 && !strncmp("fill\n", line, 5)) {
            size_t nrows = 0;
            do {
                fprintf(fout, "Row count: ");
                fgets(line, MAX_LINE_SIZE, fin);
                if (parse_uint(line, &nrows) && nrows != 0) break;
            } while (1);
            for (size_t i = 0; i < nrows; i++) {
                fprintf(fout, "[%zu]:\n", i);
                // get each field on its line
                fgets(line, MAX_LINE_SIZE, fin);
            }
        }
        else {
            fprintf(fout, "%s", line); // contains \n
        }
    }
    if (table != NULL) table_free(table);
    return EXIT_SUCCESS;
}
