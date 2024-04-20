#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "table.h"

typedef int (*cmp_func)(void const *, void const *);

#define CMP(field, type, mul) static int \
	cmp_##field##_##type(void const *a0, void const *b0) { \
		dbrow_t const *a = (dbrow_t const *)a0; \
		dbrow_t const *b = (dbrow_t const *)b0; \
		return (mul) * ( ((a->field - b->field) > 0) - ((a->field - b->field) < 0) ); \
	}
#define STR_CMP(field, type, mul, maxlen) static int cmp_##field##_##type(void const *a, void const *b) { \
		char const *str1 = ( (dbrow_t const *)a )->field; \
		char const *str2 = ( (dbrow_t const *)b )->field; \
		return (mul) * strncmp(str1, str2, maxlen); \
	}
#define ASC(field) CMP(field, asc, +1)
#define DESC(field) CMP(field, desc, -1)
#define STR_ASC(field, maxlen) STR_CMP(field, asc, +1, maxlen)
#define STR_DESC(field, maxlen) STR_CMP(field, desc, -1, maxlen)
#define Q(field) ASC(field) DESC(field)
#define STR_Q(field, maxlen) STR_ASC(field, maxlen) STR_DESC(field, maxlen)

Q(id)
Q(c1)
Q(c2)
STR_Q(c3, 17)
Q(c4)
STR_Q(c5, 33)

/*
 * returns 1 on success, 0 on failure
 * performance is not guaranteed on large tables
 */
int table_sort(table_t const *table, table_sort_t sortspec, dbrow_t **out_result) {
	if (table == NULL || table->rows == NULL || table->len == 0 || out_result == NULL) return 0;
	int desc = sortspec.direction == S_DESC;
	cmp_func cmp = NULL;
	switch (sortspec.column) {
	default: return 0;
	case TC_ID: cmp = desc ? cmp_id_desc : cmp_id_asc; break;
	case TC_C1: cmp = desc ? cmp_c1_desc : cmp_c1_asc; break;
	case TC_C2: cmp = desc ? cmp_c2_desc : cmp_c2_asc; break;
	case TC_C3: cmp = desc ? cmp_c3_desc : cmp_c3_asc; break;
	case TC_C4: cmp = desc ? cmp_c4_desc : cmp_c4_asc; break;
	case TC_C5: cmp = desc ? cmp_c5_desc : cmp_c5_asc; break;
	}
	size_t len = table->len;
	dbrow_t *rows = malloc(sizeof(dbrow_t) * len);
	if (rows == NULL) return 0;
	memcpy(rows, table->rows, sizeof(dbrow_t) * len);
	qsort(rows, len, sizeof(dbrow_t), cmp);
	*out_result = rows;
	return 1;
}
