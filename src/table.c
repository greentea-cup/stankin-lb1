#include <stdlib.h>

#include "table.h"

// https://stackoverflow.com/a/466242/20935957
// https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
static size_t npow2(size_t v) {
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

table_t *table_new(size_t cap) {
	size_t cap2 = npow2(cap);
	if (cap2 < cap || cap2 == 0) goto bad_cap;
	table_t *table = malloc(sizeof(table_t));
	if (table == NULL) goto no_table;
	dbrow_t *rows = calloc(cap2, sizeof(dbrow_t));
	if (rows == NULL) goto no_rows;
	table->rows = rows;
	table->len = 0;
	table->cap = cap2;
	table->next_id = 1;
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

int table_append(table_t *table, dbrow_t row) {
	if (table == NULL) return 0;
	if (table->rows == NULL) {
		table->rows = calloc(16, sizeof(dbrow_t));
		if (table->rows == NULL) return 0;
		table->cap = 16;
		table->len = 0;
	}
	if (table->len == table->cap) {
		dbrow_t *newrows = realloc(table->rows, table->cap * 2 * sizeof(dbrow_t));
		if (newrows == NULL) return 0;
		table->rows = newrows;
		table->cap *= 2;
	}
	table->rows[table->len++] = row;
	return 1;
}

int table_remove_at(table_t *table, size_t pos) {
	if (table == NULL || table->rows == NULL || table->len == 0 || pos >= table->len) return 0;
	for (size_t i = pos; i < table->len - 1; i++) {
		table->rows[i] = table->rows[i + 1];
	}
	table->len--;
	return 1;
}

