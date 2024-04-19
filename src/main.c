#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "table.h"
#include "parse.h"
#include "get.h"
#include "defs.h"

/*
 * returns 1 on success, 0 on failure
 * retries `interactive` times with each field
 * gets id automatically if interactive
 */
int add_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, char *line, int interactive) {
	size_t id;
	if (interactive) { id = table->next_id; }
	else if (get_id(fin, fout, ferr, line, NULL, NULL, interactive, &id) == 0) { goto add_cancel; }
	dbrow_t row = {.id = id};
	if (get_c1(fin, fout, ferr, line, NULL, NULL, interactive, &row.c1) == 0) { goto add_cancel; }
	if (get_c2(fin, fout, ferr, line, NULL, NULL, interactive, &row.c2) == 0) { goto add_cancel; }
	if (get_c3(fin, fout, ferr, line, NULL, NULL, interactive, row.c3) == 0) { goto add_cancel; }
	if (get_c4(fin, fout, ferr, line, NULL, NULL, interactive, &row.c4) == 0) { goto add_cancel; }
	if (get_c5(fin, fout, ferr, line, NULL, NULL, interactive, row.c5) == 0) { goto add_cancel; }
	table_append(table, row);
	if (interactive) { table->next_id++; }
	return 1;
add_cancel:
	afprintf(fout, "Cancelled\n");
	return 0;
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

#define ROW_HUMAN_FORMAT "%zu\t%zd\t%f\t'%s'\t%s\t'%s'\n"
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
		"\twhere\tsearch\tSearch for specific values\n"
		"\torder\tsort\tSort rows by criterion\n"
		"\tprint\t\tPrint table\n"
		"\tsave\texport\tSave table to file\n"
		"\tload\timport\tLoad table from file\n"
		"========\n"
	);
	return 1;
}

void fill_table(FILE *fin, FILE *fout, FILE *ferr, char *line, int retries, table_t **table) {
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
			return;
		}
	}
	if (*table != NULL) table_free(*table);
	*table = newtable;
}

void filter_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, char *line,
	int retries) {
	table_find_t findspec = {0};
	size_t colnum;
	get_uint(fin, fout, ferr, line,
		"Column num[uint 0-5, other for exit]: ", "Uint expected", retries, &colnum);
	switch (colnum) {
	default: afprintf(fout, "Cancelled\n"); return;
	case 0: findspec.column = TC_ID; break;
	case 1: findspec.column = TC_C1; break;
	case 2: findspec.column = TC_C2; break;
	case 3: findspec.column = TC_C3; break;
	case 4: findspec.column = TC_C4; break;
	case 5: findspec.column = TC_C5; break;
	}
	int rt = retries;
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
		if (rt == 0) { afprintf(ferr, "Max retries exceeded\n"); return; }
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
		if (rt == 0) { afprintf(ferr, "Max retries exceeded\n"); return; }
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
}

void sort_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, char *line,
	int retries) {
	table_sort_t sortspec = {0};
	size_t colnum;
	get_uint(fin, fout, ferr, line,
		"Column num[uint 0-5, other for exit]: ", "Uint expected", retries, &colnum);
	switch (colnum) {
	default: afprintf(fout, "Cancelled\n"); return;
	case 0: sortspec.column = TC_ID; break;
	case 1: sortspec.column = TC_C1; break;
	case 2: sortspec.column = TC_C2; break;
	case 3: sortspec.column = TC_C3; break;
	case 4: sortspec.column = TC_C4; break;
	case 5: sortspec.column = TC_C5; break;
	}
	int rt = retries;
	do {
		afprintf(fout, "Order[asc + desc -]: ");
		fgets(line, MAX_LINE_SIZE, fin);
		if (line[0] == '\n') { afprintf(fout, "Cancelled\n"); return; }
		else if (PROMPT("asc") || PROMPT("ASC") || PROMPT("+")) { sortspec.direction = S_ASC; break; }
		else if (PROMPT("desc") || PROMPT("DESC") || PROMPT("-")) { sortspec.direction = S_DESC; break; }
		else { afprintf(ferr, "Order: asc + desc - expected\n"); }
	}
	while (rt--);
	if (rt == 0) { afprintf(ferr, "Max retries exceeded\n"); return; }
	dbrow_t *sorted;
	table_sort(table, sortspec, &sorted);
	afprintf(fout, ROW_HEADER);
	for (size_t i = 0, len = table->len; i < len; i++) {
		afprintf(fout, ROW_HUMAN_FORMAT, ROW_ARG(sorted[i]));
	}
	free(sorted);
}

void export_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, char *line,
	int retries) {
	FILE *fsave = NULL;
	do {
		afprintf(fout, "Path: ");
		fgets(line, MAX_LINE_SIZE, fin);
		if (strlen(line) == 1) { afprintf(fout, "Cancelled\n"); return; }
		size_t i = 0;
		while (line[i] != '\n' && line[i] != '\0' && i < MAX_LINE_SIZE) i++;
		line[i] = '\0';
		fsave = fopen(line, "w");
		if (fsave == NULL) { afprintf(fout, "Cannot open file '%s'\n", line); }
		else break;
	}
	while (retries--);
	if (retries == 0) { afprintf(ferr, "Max retries exceeded\n"); return; }
	afprintf(fout, "Saving current table to '%s'\n", line);
	print_table(fsave, ferr, table, 1);
	afprintf(fout, "Saved current table\n");
	if (fsave != NULL) fclose(fsave);
}

void import_table(FILE *fin, FILE *fout, FILE *ferr, table_t **table, char *line, int retries) {
	FILE *fload = NULL;
	do {
		afprintf(fout, "Path: ");
		fgets(line, MAX_LINE_SIZE, fin);
		if (strlen(line) == 1) {
			afprintf(fout, "Cancelled\n");
			return;
		}
		size_t i = 0;
		while (line[i] != '\n' && line[i] != '\0' && i < MAX_LINE_SIZE) i++;
		line[i] = '\0';
		fload = fopen(line, "r");
		if (fload == NULL) { afprintf(fout, "Cannot open file '%s'\n", line); }
		else { break; }
	}
	while (retries--);
	if (retries == 0) { afprintf(ferr, "Max retries exceeded\n"); return; }
	afprintf(fout, "Loading table from '%s'\n", line);
	load_table(fload, ferr, table, line);
	afprintf(fout, "Loaded table\n");
	if (fload != NULL) fclose(fload);
}

int main(int argc, char *argv[]) {
	// sort by column (ASC / DESC)
	FILE *fin = stdin; // no free
	FILE *fout = stdout; // no free
	FILE *ferr = stderr; // no free
	table_t *table = NULL;
	char line[MAX_LINE_SIZE] = {0};
	if (!(argc > 1 && strcmp(argv[1], "--no-menu") == 0)) print_menu(fout);
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
		else if (line[0] == '\n') {continue;}
		else if (PROMPT("q") || PROMPT("quit") || PROMPT("exit")) {
			afprintf(fout, "Quitting\n"); break;
		}
		else if (PROMPT("h") || PROMPT("help")) {
			print_menu(fout);
		}
		else if (PROMPT("f") || PROMPT("fill")) {
			fill_table(fin, fout, ferr, line, retries, &table);
		}
		else if (PROMPT("p") || PROMPT("print")) {
			print_table(fout, ferr, table, 0);
		}
		else if (PROMPT("a") || PROMPT("add")) {
			if (table == NULL) table = table_new(16);
			add_row(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("d") || PROMPT("delete")) {
			delete_row(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("w") || PROMPT("where")) {
			filter_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("o") || PROMPT("order") || PROMPT("sort")) {
			sort_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("s") || PROMPT("save") || PROMPT("export")) {
			export_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT("l") || PROMPT("load") || PROMPT("import")) {
			import_table(fin, fout, ferr, &table, line, retries);
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
		else if (PROMPT("t_q")) {
			// silent quit
			break;
		}
		else {
			afprintf(fout, "Unknown command: %s", line);
		}
	}
	if (table != NULL) table_free(table);
	return EXIT_SUCCESS;
}
