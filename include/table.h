#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
	size_t id;
	int64_t c1;
	double c2; // todo: find some float64_t
	char c3[17];
	bool c4;
	char c5[33];
} dbrow_t;

#define ROW_ARG(row) row.id, row.c1, row.c2, row.c3, row.c4 ? "TRUE" : "FALSE", row.c5

typedef union {
	size_t id;
	int64_t c1;
	double c2;
	char c3[17];
	bool c4;
	char c5[33];
} dbrow_u;

typedef struct {
	dbrow_t *rows;
	size_t len;
	size_t cap;
	size_t next_id;
} table_t;

typedef enum { S_ASC, S_DESC } sort_dir_t;
typedef enum { TC_ID, TC_C1, TC_C2, TC_C3, TC_C4, TC_C5 } column_t;
typedef enum { C_EQ, C_NEQ, C_LT, C_GT, C_LE, C_GE, C_BTW } condition_t;

typedef struct {
	column_t column;
	condition_t condition;
	dbrow_u data1;
	dbrow_u data2;
	size_t start_pos;
} table_find_t;

typedef struct {
	column_t column;
	sort_dir_t direction;
} table_sort_t;

table_t *table_new(size_t cap);

void table_free(table_t *table);

int table_find_first(table_t const *table, table_find_t findspec, size_t *out_idx);

int table_sort(table_t const *table, table_sort_t sortspec, dbrow_t **out_result);

int table_append(table_t *table, dbrow_t row);

int table_remove_at(table_t *table, size_t pos);

#endif
