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
		else if (strchr(" \n\t", *str) != NULL) { break; }
		else { return 0; }
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
	else if (str[0] == '-')(neg = 1, str++);
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

typedef struct {
	enum {TC_ID, TC_C1, TC_C2, TC_C3, TC_C4, TC_C5} column;
	enum {C_EQ, C_NEQ, C_LT, C_GT, C_LE, C_GE, C_BTW} condition;
	union {
		size_t uint_value;
		int64_t int_value;
		bool bool_value;
		char *str_value;
	} data1;
	union {
		size_t uint_value;
		int64_t int_value;
		bool bool_value;
		char *str_value;
	} data2;
	size_t start_pos;
} table_find_t;

int table_find_first(table_t *table, table_find_t findspec, size_t *out_idx) {
	if (table == NULL || table->rows == NULL || table->len == 0 || out_idx == NULL) return 0;
	if (findspec.column == TC_ID && findspec.condition == C_EQ) {
		// special case - id ASC unique
		size_t id = findspec.data1.uint_value;
		size_t start = findspec.start_pos, end = table->len - 1;
		size_t end0 = end, result_idx = (size_t) -1;
		while (result_idx == (size_t) -1 && start <= end && end <= end0) {
			size_t middle = (start + end) / 2;
			size_t middle_id = table->rows[middle].id;
			if (middle_id < id) { start = middle + 1; }
			else if (middle_id > id) { end = middle - 1; }
			else { result_idx = middle; }
		}
		if (result_idx == (size_t) -1) { return 0; }
		else {
			*out_idx = result_idx;
			return 1;
		}
	}
	// if ((findspec.column == TC_C3 || findspec.column == TC_C5)
	//	&& !(findspec.condition == C_EQ || )
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
int get_uint(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror, int interactive, size_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_uint(line, out_result) == 0) {
			if (interactive) { if (onerror != NULL) afprintf(ferr, "%s", onerror); }
			else { return 0; }
		}
		else { return 1; }
	}
	while (interactive);
	return 1;
}

int get_int(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror, int interactive, int64_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_int(line, out_result) == 0) {
			if (interactive) { if (onerror != NULL) afprintf(ferr, "%s", onerror); }
			else { return 0; }
		}
		else { return 1; }
	}
	while (interactive);
	return 1;
}

int get_float(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror, int interactive, double *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_float(line, out_result) == 0) {
			if (interactive) { if (onerror != NULL) afprintf(ferr, "%s", onerror); }
			else { return 0; }
		}
		else { return 1; }
	}
	while (interactive);
	return 1;
}

int get_bool(FILE *fin, FILE *fout, FILE *ferr, char *line, char const *prompt, char const *onerror, int interactive, bool *out_result) {
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
		else { return 0; }

	}
	while (interactive);
	return 1;
}
// end get_*

/*
 * interactive = 1 if interactive input
 * returns 1 on success, 0 on failure
 */
int add_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line, int interactive) {
	size_t id;
	if (interactive) { id = table->next_id++; }
	else if (get_uint(fin, fout, ferr, line, "id[uint]: ", "id: Uint expected\n", interactive, &id) == 0) {
		goto bad_id;
	}
	dbrow_t row = {.id = id};

	// int64_t c1;
	if (get_int(
			fin, fout, ferr, line, "c1[int]: ", "c1: Int expected\n", interactive, &row.c1
		) == 0) { goto bad_c1; }
	// double c2; // todo: find some float64_t
	if (get_float(
			fin, fout, ferr, line, "c2[float]: ", "c2: Float expected\n", interactive, &row.c2
		) == 0) { goto bad_c2; }
	// char c3[16+1]; a-zA-Z0-9
	do {
		afprintf(fout, "c3[char 16]: ");
		fgets(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != '\0' && line[i] != '\n'; i++) {
			if (i >= 16
				|| !((line[i] >= '0' && line[i] <= '9')
					|| (line[i] >= 'a' && line[i] <= 'z')
					|| (line[i] >= 'A' && line[i] <= 'Z'))) {
				good = 0;
				break;
			}
		}
		size_t end = 0;
		while (end < MAX_LINE_SIZE - 1 && line[end] != '\0' && line[end] != '\n') end++;
		if (good) {
			strncpy(row.c3, line, end); // end-1 chars ok; last char = \n
			row.c3[end] = '\0'; // replace last char with \0
			break;
		}
		else if (interactive) {
			afprintf(ferr, "c3: Max length: 16; Valid chars are 0-9 a-z A-Z\n");
		}
		else { goto bad_c3; }
	}
	while (interactive);

	// bool c4;
	if (get_bool(
			fin, fout, ferr, line, "c4[bool]: ", "c4: Valid options are: "
			"<blank = 0> 0 1 T[RUE] F[ALSE] t[rue] f[alse] ON OFF on off\n", interactive, &row.c4
		) == 0) { goto bad_c4; }
	// char c5[32+1];
	do {
		afprintf(fout, "c5[char 32]: ");
		fgets(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != '\0' && line[i] != '\n'; i++) {
			if (i >= 32
				|| !((line[i] >= '0' && line[i] <= '9')
					|| (line[i] >= 'a' && line[i] <= 'z')
					|| (line[i] >= 'A' && line[i] <= 'Z')
					|| line[i] == ' ')) {
				good = 0;
				break;
			}
		}
		size_t end = 0;
		while (end < MAX_LINE_SIZE - 1 && line[end] != '\0' && line[end] != '\n') end++;
		if (good) {
			strncpy(row.c5, line, end); // same as c3
			row.c5[end] = '\0';
			break;
		}
		else if (interactive) {
			afprintf(ferr, "c5: Max length: 31; Valid chars are 0-9 a-z A-Z \\s\n");
		}
		else { goto bad_c5; }
	}
	while (interactive);
	table_append(table, row);
	return 1;
bad_c5:
bad_c4:
bad_c3:
bad_c2:
bad_c1:
bad_id:
	return 0;
}

int delete_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line) {
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	size_t id;
	if (get_uint(fin, fout, ferr, line, "Row id: ", "id: Uint expected\n", 1, &id) == 0) { goto bad_id; }
	table_find_t findspec = {
		.condition = C_EQ,
		.column = TC_ID,
		.data1.uint_value = id,
		.data2.uint_value = 0
	};
	size_t rowpos;
	if (table_find_first(table, findspec, &rowpos) == 0) {
		afprintf(ferr, "Row with id %zu not found\n", id);
		return 0;
	}
	afprintf(fout, "Row with id %zu is at position %zu\n", id, rowpos);
	table_remove_at(table, rowpos);
	return 1;
bad_id:
	return 0;
}

int print_table(FILE *fout, FILE *ferr, table_t *table, int dump) {
	char const *HUMAN_FORMAT = "%zu\t%zd\t%f\t'%s'\t%d\t'%s'\n";
	char const *DUMP_FORMAT = "%zu\n%zd\n%f\n%s\n%d\n%s\n";
	char const *fmt = dump ? DUMP_FORMAT : HUMAN_FORMAT;
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	if (dump) {
		afprintf(fout, "%zu\n%zu\n", table->len, table->next_id);
	}
	else {
		afprintf(fout, "id\tc1\tc2\tc3\tc4\tc5\n");
	}
	for (size_t i = 0; i < table->len; i++) {
		dbrow_t row = table->rows[i];
		afprintf(fout, fmt, ROW_ARG(row));
	}
	return 1;
}

int filter_rows(void) {
	return 0;
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
	for (size_t i = 0; i < table_len; i++) { add_row(fin, NULL, ferr, table, line, 0); }
	*out_table = table;
	return 1;
}

void print_menu(FILE *fout) {
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
}

int main(void) {
	// prompt display ("> ")
	// filter
	// delete
	FILE *fin = stdin;	 // no free
	FILE *fout = stdout; // no free
	FILE *ferr = stderr; // no free
	table_t *table = NULL;
	char line[MAX_LINE_SIZE] = {0};
	print_menu(fout);
	while (1) {
		afprintf(fout, "> ");
		if (fgets(line, MAX_LINE_SIZE, fin) == NULL) {
			// don't think it's actually possible with stack-allocated `line`
			afprintf(ferr, "Get line error (fgets returned NULL)\n");
			break;
		}
		else if (ferror(fin)) { afprintf(ferr, "Everything is bad\n"); break; }
		else if (feof(fin)) { afprintf(ferr, "EOF\n"); break; }
		else if (PROMPT("q") || PROMPT("quit") || PROMPT("exit")) {
			afprintf(fout, "Quitting\n");
			break;
		}
		else if (PROMPT("h") || PROMPT("help")) { print_menu(fout); }
		else if (PROMPT("f") || PROMPT("fill")) {
			size_t nrows = 0;
			do {
				if (get_uint(
						fin, fout, ferr, line, "Row count: ",
						"Row count: Uint > 0 expected\n", 1, &nrows
					) && nrows == 0) {
					afprintf(ferr, "Row count should be > 0\n");
				}
			}
			while (nrows == 0);
			if (table != NULL) table_free(table);
			table = table_new(nrows);
			afprintf(fout, "%zu rows\n", nrows);
			for (size_t i = 0; i < nrows; i++) {
				afprintf(fout, "[%zu]:\n", i);
				if (add_row(fin, fout, ferr, table, line, 1) == 0) {
					afprintf(fout, "Early exit; unimplemented");
				}
			}
		}
		else if (PROMPT("p") || PROMPT("print")) {
			print_table(fout, ferr, table, 0);
		}
		else if (PROMPT("a") || PROMPT("add")) {
			if (table == NULL) table = table_new(16);
			if (add_row(fin, fout, ferr, table, line, 1) == 0) { afprintf(fout, "Cancelled\n"); }
		}
		else if (PROMPT("d") || PROMPT("delete")) {
			delete_row(fin, fout, ferr, table, line);
		}
		else if (PROMPT("s") || PROMPT("save") || PROMPT("export")) {
			FILE *fsave = NULL;
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
				if (fsave == NULL) { afprintf(fout, "Cannot open file '%s'\n", line); }
				else break;
			}
			while (1);
			afprintf(fout, "Saving current table to '%s'\n", line);
			print_table(fsave, ferr, table, 1);
			afprintf(fout, "Saved current table\n");
			if (fsave != NULL) fclose(fsave);
		save_cancel:;
		}
		else if (PROMPT("l") || PROMPT("load") || PROMPT("import")) {
			FILE *fload = NULL;
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
				if (fload == NULL) { afprintf(fout, "Cannot open file '%s'\n", line); }
				else break;
			}
			while (1);
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
	}
	if (table != NULL) table_free(table);
	return EXIT_SUCCESS;
}
