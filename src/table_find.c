#include <stdlib.h>
#include <string.h>

#include "table.h"

int table_find_first(table_t const *table, table_find_t findspec, size_t *out_idx) {
	if (table == NULL || table->rows == NULL || table->len == 0 || out_idx == NULL ||
		findspec.start_pos >= table->len) return 0;
	if (findspec.column == TC_ID && findspec.condition == C_EQ) {
		// special case - id ASC unique
		// use binary search
		size_t id = findspec.data1.id;
		size_t start = findspec.start_pos, end = table->len - 1;
		size_t end0 = end, result_idx = (size_t) -1;
		while (result_idx == (size_t) -1 && start <= end && end <= end0) {
			size_t middle = (start + end) / 2;
			size_t middle_id = table->rows[middle].id;
			if (middle_id < id) { start = middle + 1; }
			else if (middle_id > id) { end = middle - 1; }
			else { result_idx = middle; }
		}
		*out_idx = result_idx;
		return result_idx != (size_t) -1;
	}
	switch (findspec.column) {
	default:
		return 0;
	case TC_ID:
	case TC_C1:
	case TC_C2:
	case TC_C3:
	case TC_C4:
	case TC_C5:
		break;
	}
	switch (findspec.condition) {
	default:
		return 0;
	case C_EQ:
	case C_NEQ:
	case C_GE:
	case C_GT:
	case C_LE:
	case C_LT:
	case C_BTW:
		break;
	}
	if ((findspec.column == TC_C3 || findspec.column == TC_C5)
		&& !(findspec.condition == C_EQ || findspec.condition == C_NEQ)) return 0;

// here is the actual search
#define TFF_LOOP(cond, pre) for \
	(size_t i = findspec.start_pos, len = table->len; i < len; i++) { \
		pre; if (cond) { *out_idx = i; return 1; } \
	}
#define TFF_ROW(col) (table->rows[i]. col)
#define TFF_DATA1(col) (findspec.data1. col)
#define TFF_DATA2(col) (findspec.data2. col)
#define TFF_COND(col, cmp) TFF_LOOP((TFF_ROW(col) cmp TFF_DATA1(col)),)
#define TFF_BTW(col) TFF_LOOP( \
	((TFF_DATA1(col) <= TFF_ROW(col)) \
		&& (TFF_ROW(col) <= TFF_DATA2(col))),\
)
#define TFF_STR_EQ(col) TFF_LOOP( \
	((a == b) && (strncmp(str1, str2, a) == 0)), \
	char *str1 = TFF_ROW(col); char *str2 = TFF_DATA1(col); \
	size_t a = strlen(str1); size_t b = strlen(str2); \
)
#define TFF_STR_NEQ(col) TFF_LOOP( \
	((a != b) || (strncmp(str1, str2, a) != 0)), \
	char *str1 = TFF_ROW(col); char *str2 = TFF_DATA1(col); \
	size_t a = strlen(str1); size_t b = strlen(str2); \
)
	switch (findspec.column) {
	default: return 0;
	case TC_ID: switch (findspec.condition) {
		default: return 0;
		case C_NEQ: TFF_COND(id, !=); break;
		case C_GE: TFF_COND(id, >=); break;
		case C_GT: TFF_COND(id, >); break;
		case C_LE: TFF_COND(id, <=); break;
		case C_LT: TFF_COND(id, <); break;
		case C_BTW: TFF_BTW(id); break;
		}
	case TC_C1: switch (findspec.condition) {
		default: return 0;
		case C_EQ: TFF_COND(c1, ==); break;
		case C_NEQ: TFF_COND(c1, !=); break;
		case C_GE: TFF_COND(c1, >=); break;
		case C_GT: TFF_COND(c1, >); break;
		case C_LE: TFF_COND(c1, <=); break;
		case C_LT: TFF_COND(c1, <); break;
		case C_BTW: TFF_BTW(c1); break;
		}
	case TC_C2: switch (findspec.condition) {
		default: return 0;
		case C_EQ: TFF_COND(c2, ==); break;
		case C_NEQ: TFF_COND(c2, !=); break;
		case C_GE: TFF_COND(c2, >=); break;
		case C_GT: TFF_COND(c2, >); break;
		case C_LE: TFF_COND(c2, <=); break;
		case C_LT: TFF_COND(c2, <); break;
		case C_BTW: TFF_BTW(c2); break;
		}
	case TC_C3: switch (findspec.condition) {
		default: return 0;
		case C_EQ: TFF_STR_EQ(c3); break;
		case C_NEQ: TFF_STR_NEQ(c3); break;
		}
	case TC_C4: switch (findspec.condition) {
		default: return 0;
		case C_EQ: TFF_COND(c4, ==); break;
		case C_NEQ: TFF_COND(c4, !=); break;
		}
	case TC_C5: switch (findspec.condition) {
		default: return 0;
		case C_EQ: TFF_STR_EQ(c5); break;
		case C_NEQ: TFF_STR_NEQ(c5); break;
		}
	}
#undef TFF_STR_NEQ
#undef TFF_STR_EQ
#undef TFF_BTW
#undef TFF_COND
#undef TFF_DATA2
#undef TFF_DATA1
#undef TFF_ROW
#undef TFF_LOOP
	return 0;
}
