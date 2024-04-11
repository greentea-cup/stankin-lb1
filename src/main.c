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

#define afprintf(stream, ...) if (stream != NULL) fprintf(stream, __VA_ARGS__)

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

// returns 1 on success and result in out_result, 0 on failure
// success if str is in format /^\+?[0-9]+\s*$/
int parse_uint(char *str, size_t *out_result) {
	if (str == NULL || str[0] == '\0' || out_result == NULL) return 0;
	if (str[0] == '+') str++;
	size_t res = 0;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			res *= 10;
			res += *str;
			res -= 48;
			str++;
		}
		else if (strchr(" \n\t", *str) != NULL) { break; }
		else { return 0; }
	}
	*out_result = res;
	return 1;
}

// returns 1 on success and result in out_result, 0 on failure
// success if str is in format /^(\+|-)?[0-9]+\s*$/
int parse_int(char *str, int64_t *out_result) {
	if (str == NULL || str[0] == '\0' || out_result == NULL) return 0;
	int neg = 0;
	if (str[0] == '+') str++;
	else if (str[0] == '-') (neg = 1, str++);
	int64_t res = 0;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			res *= 10;
			res += *str;
			res -= 48;
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
	else if (str[0] == '-') (neg = 1, str++);
	size_t full = 0, dec = 0, dv = 1;
	while (*str != '\0') {
		if (*str >= '0' && *str <= '9') {
			full *= 10;
			full += *str;
			full -= 48;
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
				dec *= 10;
				dv *= 10;
				dec += *str;
				dec -= 48;
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
#define ROW_FMT(row) row.id, row.c1, row.c2, row.c3, row.c4, row.c5

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

void table_free(table_t *table) {
	free(table->rows);
	free(table);
}

int add_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line, int manual) {
	size_t id;
	if (manual) { id = table->next_id++; }
	else {
		afprintf(fout, "id[uint]: ");
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_uint(line, &id) == 0) {
			afprintf(ferr, "Bad id; using auto\n");
			return 0;
		}
	}
	dbrow_t row;

	// int64_t c1;
	afprintf(fout, "c1[int]: ");
	do {
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_int(line, &row.c1) == 1) { break; }
		else if (manual) { afprintf(ferr, "c1: Int expected\nc1:\n"); }
		else { goto bad_c1; }
	}
	while (manual);

	// double c2; // todo: find some float64_t
	afprintf(fout, "c2[float]: ");
	do {
		fgets(line, MAX_LINE_SIZE, fin);
		if (parse_float(line, &row.c2) == 1) { break; }
		else if (manual) { afprintf(ferr, "c2: Float expected\nc2:\n"); }
		else { goto bad_c2; }
	}
	while (manual);

	// char c3[16]; a-zA-Z0-9
	afprintf(fout, "c3[char 16]: ");
	do {
		fgets(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != '\0' && line[i] != '\n'; i++) {
			if (!((line[i] >= '0' && line[i] <= '9') || (line[i] >= 'a' && line[i] <= 'z') ||
					(line[i] >= 'A' && line[i] <= 'Z')) ||
				i >= 16) {
				good = 0;
				break;
			}
		}
		size_t end = 0;
		while (end < MAX_LINE_SIZE - 1 && line[end] != '\0' && line[end] != '\n') end++;
		if (good) {
			strncpy(row.c3, line, end);
			afprintf(fout, "c3 end = %zu\n", end);
			row.c3[end] = '\0';
			break;
		}
		else if (manual) {
			afprintf(ferr, "c3: Max length: 16; Valid chars are 0-9 a-z A-Z\nc3: ");
		}
		else { goto bad_c3; }
	}
	while (manual);

	// bool c4;
	afprintf(fout, "c4[bool]: ");
	do {
		fgets(line, MAX_LINE_SIZE, fin);
		if (strlen(line) == 1 && !strncmp("\n", line, 1)) {
			row.c4 = 0;
			break;
		}
		else if (strlen(line) == 2 && !strncmp("0\n", line, 2)) {
			row.c4 = 0;
			break;
		}
		else if (strlen(line) == 2 && !strncmp("1\n", line, 2)) {
			row.c4 = 1;
			break;
		}
		else if (strlen(line) == 3 && !strncmp("ON\n", line, 3)) {
			row.c4 = 1;
			break;
		}
		else if (strlen(line) == 4 && !strncmp("OFF\n", line, 4)) {
			row.c4 = 0;
			break;
		}
		else if (manual) { afprintf(ferr, "c4: ON|OFF|0|1\nc4:\n"); }
		else { goto bad_c4; }
	}
	while (manual);

	// char c5[32];
	afprintf(fout, "c5[char 32]: ");
	do {
		fgets(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != '\0' && line[i] != '\n'; i++) {
			if (!((line[i] >= '0' && line[i] <= '9') || (line[i] >= 'a' && line[i] <= 'z') ||
					(line[i] >= 'A' && line[i] <= 'Z') || line[i] == ' ') ||
				i >= 32) {
				good = 0;
				break;
			}
		}
		size_t end = 0;
		while (end < MAX_LINE_SIZE - 1 && line[end] != '\0' && line[end] != '\n') end++;
		if (good) {
			strncpy(row.c5, line, end);
			row.c5[end] = '\0';
			break;
		}
		else if (manual) {
			afprintf(ferr, "c5: Max length: 31; Valid chars are 0-9 a-z A-Z \\s\nc5: ");
		}
		else { goto bad_c5; }
	}
	while (manual);

	table_append(table, row);

	return 1;

bad_c5:
bad_c4:
bad_c3:
bad_c2:
bad_c1:
	return 0;
}

int print_table(FILE *fout, FILE *ferr, table_t *table) {
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	afprintf(fout, "id\tc1\tc2\tc3\tc4\tc5\n");
	for (size_t i = 0; i < table->len; i++) {
		dbrow_t row = table->rows[i];
		afprintf(fout, "%zu\t%zd\t%f\t'%s'\t%d\t'%s'\n", ROW_FMT(row));
	}
	return 1;
}

int dump_table(FILE *fout, FILE *ferr, table_t *table) {
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, "No table\n");
		return 0;
	}
	afprintf(fout, "%zu\n%zu\n", table->len, table->next_id);
	for (size_t i = 0; i < table->len; i++) {
		dbrow_t row = table->rows[i];
		afprintf(fout, "%zu\n%zd\n%f\n%s\n%d\n%s\n", ROW_FMT(row));
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
	for (size_t i = 0; i < table_len; i++) { add_row(fin, NULL, ferr, table, line, 0); }
	*out_table = table;
	return 1;
}

int main(void) {
	FILE *fin = stdin;	 // no free
	FILE *fout = stdout; // no free
	FILE *ferr = stderr; // no free
	table_t *table = NULL;
	char line[MAX_LINE_SIZE] = {0};
	while (1) {
		if (fgets(line, MAX_LINE_SIZE, fin) == NULL) {
			afprintf(ferr, "Get line error (fgets returned NULL)\n");
			break;
		}
		if (ferror(fin)) {
			afprintf(ferr, "Everything is bad\n");
			break;
		}
		else if (feof(fin)) {
			afprintf(ferr, "EOF\n");
			break;
		}
		else if (strlen(line) == 5 && !strncmp("quit\n", line, 5)) {
			afprintf(fout, "Quitting\n");
			break;
		}
		else if (strlen(line) == 5 && !strncmp("fill\n", line, 5)) {
			size_t nrows = 0;
			do {
				afprintf(fout, "Row count: ");
				fgets(line, MAX_LINE_SIZE, fin);
				if (parse_uint(line, &nrows) && nrows != 0) break;
			}
			while (1);
			if (table != NULL) table_free(table);
			table = table_new(nrows);
			afprintf(fout, "%zu rows\n", nrows);
			for (size_t i = 0; i < nrows; i++) {
				afprintf(fout, "[%zu]:\n", i);
				if (!add_row(fin, fout, ferr, table, line, 1)) {
					afprintf(fout, "Early exit; unimplemented");
				}
			}
		}
		else if (strlen(line) == 2 && !strncmp("i\n", line, 2)) {
			int64_t testi;
			afprintf(fout, "Int: ");
			fgets(line, MAX_LINE_SIZE, fin);
			if (parse_int(line, &testi)) afprintf(fout, "%zd\n", testi);
		}
		else if (strlen(line) == 2 && !strncmp("f\n", line, 2)) {
			double testf;
			afprintf(fout, "Float: ");
			fgets(line, MAX_LINE_SIZE, fin);
			if (parse_float(line, &testf)) afprintf(fout, "%f\n", testf);
		}
		else if (strlen(line) == 6 && !strncmp("print\n", line, 6)) {
			print_table(fout, ferr, table);
		}
		else if (strlen(line) == 4 && !strncmp("add\n", line, 4)) {
			if (table == NULL) table = table_new(16);
			if (!add_row(fin, fout, ferr, table, line, 1)) { afprintf(fout, "Cancelled\n"); }
		}
		else if (strlen(line) == 5 && !strncmp("save\n", line, 5)) {
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
			dump_table(fsave, ferr, table);
			afprintf(fout, "Saved current table\n");
			if (fsave != NULL) fclose(fsave);
save_cancel:;
		}
		else if (strlen(line) == 5 && !strncmp("load\n", line, 5)) {
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
		else {
			afprintf(fout, "%s", line); // contains \n
		}
	}
	if (table != NULL) table_free(table);
	return EXIT_SUCCESS;
}
