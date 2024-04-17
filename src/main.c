#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

// Note: both gcc and clang optimize away strlen calls on static string literals
// msvc does it only with /O2, so x64-debug may be a bit unoptimized
#define PROMPT(x) (strlen(line) == strlen(x "\n") && !strncmp(x "\n", line, strlen(x "\n")))
#define afprintf(stream, ...) { if (stream != NULL) fprintf(stream, __VA_ARGS__); }

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

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^\+?[0-9]+\s*$/
 */
int parse_uint(char *str, size_t *out_result) {
	if (str == NULL || str[0] == '\0' || out_result == NULL) return 0;
	if (str[0] == '+') str++;
	size_t res = 0;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			res = res * 10 + *str - '0';
			str++;
		}
		else if (strchr(" \n\t", *str) != NULL) {
			break;
		}
		else {
			return 0;
		}
	}
	*out_result = res;
	return 1;
}

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^(\+|-)?[0-9]+\s*$/
 */
int parse_int(char *str, int64_t *out_result) {
	if (str == NULL || str[0] == '\0' || out_result == NULL) return 0;
	int neg = 0;
	if (str[0] == '+') str++;
	else if (str[0] == '-') { neg = 1, str++; };
	int64_t res = 0;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			res = res * 10 + *str - '0';
			str++;
		}
		else if (strchr(" \n\t", *str) != NULL) { break; }
		else { return 0; }
	}
	*out_result = neg ? -res : res;
	return 1;
}

int parse_float(char *str, double *out_result) {
	if (str == NULL || str[0] == '\0' || out_result == NULL) return 0;
	int neg = 0;
	if (str[0] == '+') str++;
	else if (str[0] == '-') { neg = 1; str++; }
	size_t full = 0, dec = 0, dv = 1;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			full = full * 10 + *str - '0';
			str++;
		}
		else if (*str == '.') { break; }
		else if (strchr(" \n\t", *str) != NULL) { break; }
		else { return 0; }
	}
	if (*str == '.') {
		str++;
		while (*str != '\0') {
			if (*str >= '0' && *str <= '9') {
				dv *= 10;
				dec = dec * 10 + *str - '0';
				str++;
			}
			else if (strchr(" \n\t", *str) != NULL) { break; }
			else { return 0; }
		}
	}
	double res = full + (double)dec / dv;
	*out_result = neg ? -res : res;
	return 1;
}

typedef struct {
	size_t id;
	int64_t c1;
	double c2; // todo: find some float64_t
	char c3[17];
	bool c4;
	char c5[33];
} dbrow_t;

#define ROW_ARG(row) row.id, row.c1, row.c2, row.c3, row.c4, row.c5

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
typedef enum {TC_ID, TC_C1, TC_C2, TC_C3, TC_C4, TC_C5} column_t;
typedef enum {C_EQ, C_NEQ, C_LT, C_GT, C_LE, C_GE, C_BTW} condition_t;

typedef struct {
	column_t column;
	condition_t condition;
	dbrow_u data1;
	dbrow_u data2;
	size_t start_pos;
} table_find_t;

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

int table_remove_at(table_t *table, size_t pos) {
	if (table == NULL || table->rows == NULL || table->len == 0 || pos >= table->len) return 0;
	for (size_t i = pos; i < table->len - 1; i++) {
		table->rows[i] = table->rows[i + 1];
	}
	table->len--;
	return 1;
}

void table_free(table_t *table) {
	free(table->rows);
	free(table);
}

// get_*
/*
 * interactive = 1 if interactive input
 * returns 1 on success, 0 on failure
 */
int get_uint(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive,
	size_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_uint(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, "%s", onerror);
			}
			else {
				return 0;
			}
		}
		else {
			return 1;
		}
	}
	while (interactive--);
	return 1;
}

int get_int(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive,
	int64_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_int(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, "%s", onerror);
			}
			else {
				return 0;
			}
		}
		else {
			return 1;
		}
	}
	while (interactive--);
	return 1;
}

int get_float(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt,
	char const *onerror, int interactive,
	double *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_float(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, "%s", onerror);
			}
			else {
				return 0;
			}
		}
		else {
			return 1;
		}
	}
	while (interactive--);
	return 1;
}

int get_bool(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, bool *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (PROMPT("") || PROMPT("0") || PROMPT("F") || PROMPT("f")
			|| PROMPT("OFF") || PROMPT("off")
			|| PROMPT("FALSE") || PROMPT("false")) {
			*out_result = 0;
			break;
		}
		else if (PROMPT("1") || PROMPT("T") || PROMPT("t")
			|| PROMPT("ON") || PROMPT("on")
			|| PROMPT("TRUE") || PROMPT("true")) {
			*out_result = 1;
			break;
		}
		else if (interactive) {
			afprintf(ferr, "%s", onerror);
		}
		else {
			return 0;
		}

	}
	while (interactive--);
	return 1;
}

/*
 * returns 1 on success, 0 on failure
 * retries interactive times
 * out_result should have capacity of maxlen + 1 characters (maxlen in chars + '\0')
 */
int get_str(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, char const *whitelist, size_t maxlen, char *out_result) {
	int ii = interactive;
	do {
		afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != '\0' && line[i] != '\n'; i++) {
			if (i >= maxlen || strchr(whitelist, line[i]) == NULL) {
				good = 0;
				break;
			}
		}
		if (good) {
			size_t end = 0;
			while (end < MAX_LINE_SIZE - 1 && end < maxlen && line[end] != '\0' && line[end] != '\n') end++;
			strncpy(out_result, line, end);
			out_result[end] = '\0';
			return 1;
		}
		else if (interactive) {
			afprintf(ferr, "%s", onerror);
		}
		else {
			return 0;
		}
	}
	while (interactive--);
	if (ii) {
		afprintf(ferr, "Max retries exceeded\n");
		afprintf(fout, "Cancelled\n");
	}
	return 0;
}

#define DIGITS "0123456789"
#define ALPH_EN_LOW "abcdefghijklmnopqrstuvwxyz"
#define ALPH_EN_UPP "abcdefghijklmnopqrstuvwxyz"

int get_id(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, size_t *out_result) {
	return get_uint(fin, fout, ferr, line, prompt != NULL ? onerror : "id[uint]: ",
			onerror != NULL ? onerror : "id: Uint expected\n", interactive,
			out_result);
}

int get_c1(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, int64_t *out_result) {
	return get_int(fin, fout, ferr, line, prompt != NULL ? prompt : "c1[int]: ",
			onerror != NULL ? onerror : "c1: Int expected\n", interactive, out_result);
}

int get_c2(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, double *out_result) {
	return get_float(fin, fout, ferr, line, prompt != NULL ? prompt : "c2[float]: ",
			onerror != NULL ? onerror : "c2: Float expected\n", interactive,
			out_result);
}

int get_c3(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, char *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : "c3[char 16]: ",
			onerror != NULL ? onerror : "c3: Max length: 16; Valid chars are 0-9 a-z A-Z\n", interactive,
			DIGITS ALPH_EN_LOW ALPH_EN_UPP, 16, out_result);
}

int get_c4(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, bool *out_result) {
	return get_bool(fin, fout, ferr, line, prompt != NULL ? prompt : "c4[bool]: ",
			onerror != NULL ? onerror : "c4: Valid options are: "
			"<blank = 0> 0 1 T[RUE] F[ALSE] t[rue] f[alse] ON OFF on off\n", interactive, out_result);
}

int get_c5(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror,
	int interactive, char *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : "c5[char 32]: ",
			onerror != NULL ? onerror : "c5: Max length: 31; Valid chars are 0-9 a-z A-Z \\s\n", interactive,
			DIGITS ALPH_EN_LOW ALPH_EN_UPP " ", 32, out_result);
}
// end get_*

/*
 * returns 1 on success, 0 on failure
 * retries `interactive` times with each field
 * gets id automatically if interactive
 */
int add_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line, int interactive) {
	size_t id;
	if (interactive) { id = table->next_id; }
	else if (get_id(fin, fout, ferr, line, NULL, NULL, interactive, &id) == 0) { return 0; }
	dbrow_t row = {.id = id};
	if (get_c1(fin, fout, ferr, line, NULL, NULL, interactive, &row.c1) == 0) { return 0; }
	if (get_c2(fin, fout, ferr, line, NULL, NULL, interactive, &row.c2) == 0) { return 0; }
	if (get_c3(fin, fout, ferr, line, NULL, NULL, interactive, row.c3) == 0) { return 0; }
	if (get_c4(fin, fout, ferr, line, NULL, NULL, interactive, &row.c4) == 0) { return 0; }
	if (get_c5(fin, fout, ferr, line, NULL, NULL, interactive, row.c5) == 0) { return 0; }
	table_append(table, row);
	if (interactive) { table->next_id++; }
	return 1;
}

int delete_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line, int interactive) {
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	size_t id;
	if (get_id(fin, fout, ferr, line, NULL, NULL, interactive, &id) == 0) { return 0; }
	table_find_t findspec = {
		.condition = C_EQ,
		.column = TC_ID,
		.data1.id = id,
	};
	size_t rowpos;
	if (table_find_first(table, findspec, &rowpos) == 0) {
		afprintf(ferr, "Row with id %zu not found\n", id);
		return 0;
	}
	afprintf(fout, "Row with id %zu is at position %zu\n", id, rowpos);
	table_remove_at(table, rowpos);
	return 1;
}

#define ROW_HUMAN_FORMAT "%zu\t%zd\t%f\t'%s'\t%d\t'%s'\n"
#define ROW_DUMP_FORMAT "%zu\n%zd\n%f\n%s\n%d\n%s\n"
#define ROW_HEADER "id\tc1\tc2\tc3\tc4\tc5\n"

int print_table(FILE *fout, FILE *ferr, table_t const *table, int dump) {
	char const *fmt = dump ? ROW_DUMP_FORMAT : ROW_HUMAN_FORMAT;
	if (table == NULL || table->rows == NULL || table->len == 0) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	if (dump) {
		afprintf(fout, "%zu\n%zu\n", table->len, table->next_id);
	}
	else {
		afprintf(fout, ROW_HEADER);
	}
	for (size_t i = 0; i < table->len; i++) {
		dbrow_t row = table->rows[i];
		afprintf(fout, fmt, ROW_ARG(row));
	}
	return 1;
}

int print_matching_rows(FILE *fout, FILE *ferr, table_t const *table, table_find_t findspec) {
	if (table == NULL || table->rows == NULL || table->len == 0) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	afprintf(fout, ROW_HEADER);
	size_t rowidx;
	int hasNext = table_find_first(table, findspec, &rowidx);
	while (hasNext) {
		afprintf(fout, ROW_HUMAN_FORMAT, ROW_ARG(table->rows[rowidx]));
		findspec.start_pos = rowidx + 1;
		hasNext = table_find_first(table, findspec, &rowidx);
	}
	return 1;
}

int load_table(FILE *fin, FILE *ferr, table_t **out_table, char *line) {
	// no error checking
	size_t table_len = 0, table_next_id = 0;
	fgets(line, MAX_LINE_SIZE, fin);
	parse_uint(line, &table_len);
	fgets(line, MAX_LINE_SIZE, fin);
	parse_uint(line, &table_next_id);
	table_t *table = table_new(table_len);
	table->next_id = table_next_id;
	for (size_t i = 0; i < table_len; i++) {
		add_row(fin, NULL, ferr, table, line, 0);
	}
	*out_table = table;
	return 1;
}

int print_menu(FILE *fout) {
	afprintf(fout,
		"STANKIN static database operator\n"
		"\tCommand\tAlias\tDescription\n"
		"\thelp\t\tPrint this message\n"
		"\tquit\texit\tExit\n"
		"\tfill\t\tFill table with data\n"
		"\tadd\t\tAppend row to table\n"
		"\tprint\t\tPrint table\n"
		"\tsave\texport\tSave table to file\n"
		"\tload\timport\tLoad table from file\n"
		"========\n"
	);
	return 1;
}

int main(void) {
	FILE *fin = stdin; // no free
	FILE *fout = stdout; // no free
	FILE *ferr = stderr; // no free
	table_t *table = NULL;
	char line[MAX_LINE_SIZE] = {0};
	print_menu(fout);
	int retries = 3;
	while (1) {
		afprintf(fout, "> ");
		if (fgets(line, MAX_LINE_SIZE, fin) == NULL) {
			// don't think it's actually possible with stack-allocated `line`
			afprintf(ferr, "Get line error (fgets returned NULL)\n");
			break;
		}
		else if (ferror(fin)) {
			afprintf(ferr, "Everything is bad\n"); break;
		}
		else if (feof(fin)) {
			afprintf(ferr, "EOF\n"); break;
		}
		else if (PROMPT("q") || PROMPT("quit") || PROMPT("exit")) {
			afprintf(fout, "Quitting\n"); break;
		}
		else if (PROMPT("h") || PROMPT("help")) {
			print_menu(fout);
		}
		else if (PROMPT("f") || PROMPT("fill")) {
			size_t nrows = 0;
			if (get_uint(fin, fout, ferr, line, "Row count: ",
					"Row count: Uint > 0 expected\n", retries, &nrows) && nrows == 0) {
				afprintf(ferr, "Row count should be > 0\n");
				afprintf(fout, "Cancelled\n");
			}
			table_t *newtable = table_new(nrows);
			afprintf(fout, "%zu rows\n", nrows);
			for (size_t i = 0; i < nrows; i++) {
				afprintf(fout, "[%zu]:\n", i);
				if (add_row(fin, fout, ferr, newtable, line, 1) == 0) {
					afprintf(fout, "Cancelled\n");
					table_free(newtable);
					goto fill_cancel;
				}
			}
			if (table != NULL) table_free(table);
			table = newtable;
		fill_cancel:;
		}
		else if (PROMPT("p") || PROMPT("print")) {
			print_table(fout, ferr, table, 0);
		}
		else if (PROMPT("a") || PROMPT("add")) {
			if (table == NULL) table = table_new(16);
			if (add_row(fin, fout, ferr, table, line, retries) == 0) {
				afprintf(fout, "Cancelled\n");
			}
		}
		else if (PROMPT("d") || PROMPT("delete")) {
			delete_row(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("w") || PROMPT("where")) {
			table_find_t findspec = {0};
			int rt = retries;
			do {
				size_t colnum;
				get_uint(fin, fout, ferr, line, "Column num[uint 0-5, other for exit]: ",
					"Uint expected", retries, &colnum);
				switch (colnum) {
				default:
					afprintf(fout, "Cancelled\n"); goto where_cancel;
				case 0: findspec.column = TC_ID; break;
				case 1: findspec.column = TC_C1; break;
				case 2: findspec.column = TC_C2; break;
				case 3: findspec.column = TC_C3; break;
				case 4: findspec.column = TC_C4; break;
				case 5: findspec.column = TC_C5; break;
				}
				break;
			}
			while (rt--);
			rt = retries;
			switch (findspec.column) {
			case TC_ID:
			case TC_C1:
			case TC_C2:
				// any of eq, neq, gt, ge, lt, le, btw
				do {
					afprintf(fout, "Compare method[= ! > >= < <= <>] [default =]: ");
					fgets(line, MAX_LINE_SIZE, fin);
					if (PROMPT("") || PROMPT("=") || PROMPT("eq")) { findspec.condition = C_EQ; break; }
					else if (PROMPT("!") || PROMPT("neq")) { findspec.condition = C_NEQ; break; }
					else if (PROMPT(">") || PROMPT("gt")) { findspec.condition = C_GT; break; }
					else if (PROMPT(">=") || PROMPT("ge")) { findspec.condition = C_GE; break; }
					else if (PROMPT("<") || PROMPT("lt")) { findspec.condition = C_LT; break; }
					else if (PROMPT("<=") || PROMPT("le")) { findspec.condition = C_LE; break; }
					else if (PROMPT("<>") || PROMPT("btw")) { findspec.condition = C_BTW; break; }
					else { afprintf(ferr, "Compare: = ! > >= < <= <> expected\n"); }
				}
				while (rt--);
				break;
			case TC_C3:
			case TC_C4:
			case TC_C5:
				// any of eq, neq
				do {
					afprintf(fout, "Compare method: [= !] [default =]: ");
					fgets(line, MAX_LINE_SIZE, fin);
					if (PROMPT("") || PROMPT("=") || PROMPT("eq")) { findspec.condition = C_EQ; break; }
					else if (PROMPT("!") || PROMPT("neq")) { findspec.condition = C_NEQ; break; }
					else { afprintf(ferr, "Compare: = ! expected\n"); }
				}
				while (rt--);
				break;
			}
			switch (findspec.column) {
			case TC_ID:
				get_id(fin, fout, ferr, line, "id: first operand[uint]: ", NULL, retries, &findspec.data1.id);
				if (findspec.condition == C_BTW)
					get_id(fin, fout, ferr, line, "id: second operand[uint]", NULL, retries, &findspec.data2.id);
				break;
			case TC_C1:
				get_c1(fin, fout, ferr, line, "c1: first operand[int]: ", NULL, retries, &findspec.data1.c1);
				if (findspec.condition == C_BTW)
					get_c1(fin, fout, ferr, line, "c1: second operand[int]: ", NULL, retries, &findspec.data2.c1);
				break;
			case TC_C2:
				get_c2(fin, fout, ferr, line, "c2: first operand[float]: ", NULL, retries, &findspec.data1.c2);
				if (findspec.condition == C_BTW)
					get_c2(fin, fout, ferr, line, "c2: second operand[float]: ", NULL, retries, &findspec.data2.c2);
				break;
			case TC_C3:
				get_c3(fin, fout, ferr, line, "c3: first operand[char 16]: ", NULL, retries, findspec.data1.c3);
				break;
			case TC_C4:
				get_c4(fin, fout, ferr, line, "c4: first operand[bool]: ", NULL, retries, &findspec.data1.c4);
				break;
			case TC_C5:
				get_c5(fin, fout, ferr, line, "c5: first operand[char 32]: ", NULL, retries, findspec.data1.c5);
				break;
			}
			print_matching_rows(fout, ferr, table, findspec);
		where_cancel:;
		}
		else if (PROMPT("s") || PROMPT("save") || PROMPT("export")) {
			FILE *fsave = NULL;
			int rt = retries;
			do {
				afprintf(fout, "Path: ");
				fgets(line, MAX_LINE_SIZE, fin);
				if (strlen(line) == 1) {
					afprintf(fout, "Cancelled\n");
					goto save_cancel;
				}
				size_t i = 0;
				while (line[i] != '\n' && line[i] != '\0' && i < MAX_LINE_SIZE) i++;
				line[i] = '\0';
				fsave = fopen(line, "w");
				if (fsave == NULL) {
					afprintf(fout, "Cannot open file '%s'\n", line);
				}
				else break;
			}
			while (rt--);
			afprintf(fout, "Saving current table to '%s'\n", line);
			print_table(fsave, ferr, table, 1);
			afprintf(fout, "Saved current table\n");
			if (fsave != NULL) fclose(fsave);
		save_cancel:;
		}
		else if (PROMPT("l") || PROMPT("load") || PROMPT("import")) {
			FILE *fload = NULL;
			int rt = retries;
			do {
				afprintf(fout, "Path: ");
				fgets(line, MAX_LINE_SIZE, fin);
				if (strlen(line) == 1) {
					afprintf(fout, "Cancelled\n");
					goto load_cancel;
				}
				size_t i = 0;
				while (line[i] != '\n' && line[i] != '\0' && i < MAX_LINE_SIZE) i++;
				line[i] = '\0';
				fload = fopen(line, "r");
				if (fload == NULL) {
					afprintf(fout, "Cannot open file '%s'\n", line);
				}
				else break;
			}
			while (rt--);
			afprintf(fout, "Loading table from '%s'\n", line);
			load_table(fload, ferr, &table, line);
			afprintf(fout, "Loaded current table\n");
			if (fload != NULL) fclose(fload);
		load_cancel:;
		}
		else if (PROMPT("t_i")) {
			int64_t testi;
			afprintf(fout, "Int: ");
			fgets(line, MAX_LINE_SIZE, fin);
			if (parse_int(line, &testi)) afprintf(fout, "%zd\n", testi);
		}
		else if (PROMPT("t_f")) {
			double testf;
			afprintf(fout, "Float: ");
			fgets(line, MAX_LINE_SIZE, fin);
			if (parse_float(line, &testf)) afprintf(fout, "%f\n", testf);
		}
		else if (PROMPT("t_c")) {
			for (unsigned int i = 0; i < 100; i++) {
				afprintf(fout, "\n");
			}
		}
		else {
			afprintf(fout, "Unknown command: %s", line);
		}
	}
	if (table != NULL) table_free(table);
	return EXIT_SUCCESS;
}
