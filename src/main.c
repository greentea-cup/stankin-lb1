#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "table.h"
#include "parse.h"
#include "get.h"
#include "defs.h"

FILE *wide_fopen(wchar_t const *path, wchar_t const *mode) {
#ifdef _MSC_VER
	return _wfopen(path, mode);
#else
	// usually there is some character limits in the os/fs
	// if that's not the case - feel free to redefine MAX_FILE_PATH
	// or even add heap-allocated temporary strings instead of mbname, mbmode
	char mbpath[MAX_FILE_PATH];
	char mbmode[MAX_FILE_PATH];
	mbstate_t state = {0};
	size_t len = 1 + wcsrtombs(NULL, &path, 0, &state);
	if (len >= MAX_FILE_PATH) return NULL;
	if (wcsrtombs(mbpath, &path, len, &state) == (size_t) -1) return NULL;
	state = (mbstate_t) {0};
	len = 1 + wcsrtombs(NULL, &mode, 0, &state);
	if (len >= MAX_FILE_PATH) return NULL;
	if (wcsrtombs(mbmode, &mode, len, &state) == (size_t) -1) return NULL;
	FILE *fd = fopen(mbpath, mbmode);
	if (fd != NULL) fwide(fd, 1);
	return fd;
#endif
}

/*
 * returns 1 on success, 0 on failure
 * retries `interactive` times with each field
 * gets id automatically if interactive
 */
int add_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, wchar_t *line, int interactive) {
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
	afprintf(fout, L"Cancelled\n");
	return 0;
}

int delete_row(FILE *fin, FILE *fout, FILE *ferr, table_t *table, wchar_t *line, int interactive) {
	if (table == NULL || table->rows == NULL) {
		afprintf(ferr, L"No table\n");
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
		afprintf(ferr, L"Row with id %zu not found\n", id);
		return 0;
	}
	afprintf(fout, L"Row with id %zu is at position %zu\n", id, rowpos);
	table_remove_at(table, rowpos);
	return 1;
}

#ifdef _MSC_VER
#define ROW_HUMAN_FORMAT L"%zd\t%lld\t%f\t'"WSTR_FMT"'\t%d\t'"WSTR_FMT"'\n"
#define ROW_DUMP_FORMAT L"%zd\n%lld\n%f\n"WSTR_FMT"\n%d\n"WSTR_FMT"\n"
#else
#define ROW_HUMAN_FORMAT L"%zu\t%zd\t%f\t'"WSTR_FMT"'\t%d\t'"WSTR_FMT"'\n"
#define ROW_DUMP_FORMAT L"%zu\n%zd\n%f\n"WSTR_FMT"\n%d\n"WSTR_FMT"\n"
#endif
#define ROW_HEADER L"id\tc1\tc2\tc3\tc4\tc5\n"

int print_table(FILE *fout, FILE *ferr, table_t const *table, int dump) {
	wchar_t const *fmt = dump ? ROW_DUMP_FORMAT : ROW_HUMAN_FORMAT;
	if (table == NULL || table->rows == NULL || table->len == 0) {
		afprintf(ferr, L"No table\n");
		return 0;
	}
	if (dump) {
		afprintf(fout, L"%zu\n%zu\n", table->len, table->next_id);
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
		afprintf(ferr, L"No table\n");
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

int load_table(FILE *fin, FILE *ferr, table_t **out_table, wchar_t *line) {
	if (fin == NULL || out_table == NULL) return 0;
	size_t table_len = 0, table_next_id = 0;
	fgetws(line, MAX_LINE_SIZE, fin);
	wparse_uint(line, &table_len);
	fgetws(line, MAX_LINE_SIZE, fin);
	wparse_uint(line, &table_next_id);
	table_t *table = table_new(table_len);
	table->next_id = table_next_id;
	for (size_t i = 0; i < table_len; i++) {
		if (add_row(fin, NULL, ferr, table, line, 0) == 0) {
			table_free(table);
			return 0;
		}
	}
	*out_table = table;
	return 1;
}

int print_menu(FILE *fout) {
	afprintf(fout,
		L"STANKIN static database operator\n"
		L"        Command\tAlias\tDescription\n"
		L"        help\t\tPrint this message\n"
		L"        quit\texit\tExit\n"
		L"        fill\t\tFill table with data\n"
		L"        add\t\tAppend row to table\n"
		L"        where\tsearch\tSearch for specific values\n"
		L"        order\tsort\tSort rows by criterion\n"
		L"        print\t\tPrint table\n"
		L"        save\texport\tSave table to file\n"
		L"        load\timport\tLoad table from file\n"
		L"========\n"
	);
	return 1;
}

void fill_table(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, int retries, table_t **table) {
	if (table == NULL) return;
	size_t nrows = 0;
	if (get_uint(fin, fout, ferr, line, L"Row count: ",
			L"Row count: Uint > 0 expected\n", retries, &nrows) && nrows == 0) {
		afprintf(ferr, L"Row count should be > 0\n");
		afprintf(fout, L"Cancelled\n");
	}
	table_t *newtable = table_new(nrows);
	afprintf(fout, L"%zu rows\n", nrows);
	for (size_t i = 0; i < nrows; i++) {
		afprintf(fout, L"[%zu]:\n", i);
		if (add_row(fin, fout, ferr, newtable, line, 1) == 0) {
			afprintf(fout, L"Cancelled\n");
			table_free(newtable);
			return;
		}
	}
	if (*table != NULL) table_free(*table);
	*table = newtable;
}

void filter_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, wchar_t *line,
	int retries) {
	table_find_t findspec = {0};
	size_t colnum;
	get_uint(fin, fout, ferr, line,
		L"Column num[uint 0-5, other for exit]: ", L"Uint expected", retries, &colnum);
	if (colnum > TC_C5) {
		afprintf(fout, L"Cancelled\n");
		return;
	}
	else {
		findspec.column = TC_ID + colnum;
	}
	int rt = retries;
	switch (findspec.column) {
	case TC_ID:
	case TC_C1:
	case TC_C2:
		// any of eq, neq, gt, ge, lt, le, btw
		do {
			afprintf(fout, L"Compare method[= ! > >= < <= <>] [default =]: ");
			fgetws(line, MAX_LINE_SIZE, fin);
			if (PROMPT(L"") || PROMPT(L"=") || PROMPT(L"eq")) { findspec.condition = C_EQ; break; }
			else if (PROMPT(L"!") || PROMPT(L"neq")) { findspec.condition = C_NEQ; break; }
			else if (PROMPT(L">") || PROMPT(L"gt")) { findspec.condition = C_GT; break; }
			else if (PROMPT(L">=") || PROMPT(L"ge")) { findspec.condition = C_GE; break; }
			else if (PROMPT(L"<") || PROMPT(L"lt")) { findspec.condition = C_LT; break; }
			else if (PROMPT(L"<=") || PROMPT(L"le")) { findspec.condition = C_LE; break; }
			else if (PROMPT(L"<>") || PROMPT(L"btw")) { findspec.condition = C_BTW; break; }
			else { afprintf(ferr, L"Compare: = ! > >= < <= <> expected\n"); }
		}
		while (rt--);
		if (rt == 0) { afprintf(ferr, L"Max retries exceeded\n"); return; }
		break;
	case TC_C3:
	case TC_C4:
	case TC_C5:
		// any of eq, neq
		do {
			afprintf(fout, L"Compare method: [= !] [default =]: ");
			fgetws(line, MAX_LINE_SIZE, fin);
			if (PROMPT(L"") || PROMPT(L"=") || PROMPT(L"eq")) { findspec.condition = C_EQ; break; }
			else if (PROMPT(L"!") || PROMPT(L"neq")) { findspec.condition = C_NEQ; break; }
			else { afprintf(ferr, L"Compare: = ! expected\n"); }
		}
		while (rt--);
		if (rt == 0) { afprintf(ferr, L"Max retries exceeded\n"); return; }
		break;
	}
#define FT_GET_(enumv, col, type, amp) case enumv: \
		get_##col(fin, fout, ferr, line, L ## #col L": first operand[" L ## #type L"]: ", NULL, retries, amp findspec.data1.col);
#define FT_GET1(...) FT_GET_(__VA_ARGS__); break;
#define FT_GET2(enumv, col, type, amp) FT_GET_(enumv, col, type, amp); \
	if (findspec.condition == C_BTW) \
		get_##col(fin, fout, ferr, line, L ## #col L": second operand[" L ## #type L"]: ", NULL, retries, amp findspec.data2.col); \
	break;
	switch (findspec.column) {
		FT_GET2(TC_ID, id, uint, &);
		FT_GET2(TC_C1, c1, uint, &);
		FT_GET2(TC_C2, c2, uint, &);
		FT_GET1(TC_C3, c3, uint,);
		FT_GET1(TC_C4, c4, uint, &);
		FT_GET1(TC_C5, c5, uint,);
	}
#undef FT_GET_
#undef FT_GET1
#undef FT_GET2
	print_matching_rows(fout, ferr, table, findspec);
}

void sort_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, wchar_t *line,
	int retries) {
	if (table == NULL) {
		afprintf(ferr, L"No table\n");
		return;
	}
	table_sort_t sortspec = {0};
	size_t colnum;
	get_uint(fin, fout, ferr, line,
		L"Column num[uint 0-5, other for exit]: ", L"Uint expected", retries, &colnum);
	switch (colnum) {
	default: afprintf(fout, L"Cancelled\n"); return;
	case 0: sortspec.column = TC_ID; break;
	case 1: sortspec.column = TC_C1; break;
	case 2: sortspec.column = TC_C2; break;
	case 3: sortspec.column = TC_C3; break;
	case 4: sortspec.column = TC_C4; break;
	case 5: sortspec.column = TC_C5; break;
	}
	int rt = retries;
	do {
		afprintf(fout, L"Order[asc + desc -]: ");
		fgetws(line, MAX_LINE_SIZE, fin);
		if (line[0] == L'\n') { afprintf(fout, L"Cancelled\n"); return; }
		else if (PROMPT(L"asc") || PROMPT(L"ASC") || PROMPT(L"+")) { sortspec.direction = S_ASC; break; }
		else if (PROMPT(L"desc") || PROMPT(L"DESC") || PROMPT(L"-")) { sortspec.direction = S_DESC; break; }
		else { afprintf(ferr, L"Order: asc + desc - expected\n"); }
	}
	while (rt--);
	if (rt == 0) { afprintf(ferr, L"Max retries exceeded\n"); return; }
	dbrow_t *sorted;
	table_sort(table, sortspec, &sorted);
	afprintf(fout, ROW_HEADER);
	for (size_t i = 0, len = table->len; i < len; i++) {
		afprintf(fout, ROW_HUMAN_FORMAT, ROW_ARG(sorted[i]));
	}
	free(sorted);
}

void export_table(FILE *fin, FILE *fout, FILE *ferr, table_t const *table, wchar_t *line,
	int retries) {
	FILE *fsave = NULL;
	do {
		afprintf(fout, L"Path: ");
		fgetws(line, MAX_LINE_SIZE, fin);
		if (wcslen(line) == 1) { afprintf(fout, L"Cancelled\n"); return; }
		size_t i = 0;
		while (line[i] != L'\n' && line[i] != L'\0' && i < MAX_LINE_SIZE) i++;
		line[i] = L'\0';
		fsave = wide_fopen(line, L"w"WFOPEN_ARG);
		if (fsave == NULL) { afprintf(fout, L"Cannot open file '"WSTR_FMT"'\n", line); }
		else break;
	}
	while (retries--);
	if (retries == 0) { afprintf(ferr, L"Max retries exceeded\n"); return; }
	afprintf(fout, L"Saving current table to '"WSTR_FMT"'\n", line);
	print_table(fsave, ferr, table, 1);
	afprintf(fout, L"Saved current table\n");
	if (fsave != NULL) fclose(fsave);
}

void import_table(FILE *fin, FILE *fout, FILE *ferr, table_t **table, wchar_t *line, int retries) {
	FILE *fload = NULL;
	do {
		afprintf(fout, L"Path: ");
		fgetws(line, MAX_LINE_SIZE, fin);
		if (wcslen(line) == 1) {
			afprintf(fout, L"Cancelled\n");
			return;
		}
		size_t i = 0;
		while (line[i] != L'\n' && line[i] != L'\0' && i < MAX_LINE_SIZE) i++;
		line[i] = L'\0';
		fload = wide_fopen(line, L"r"WFOPEN_ARG);
		if (fload == NULL) { afprintf(fout, L"Cannot open file '"WSTR_FMT"'\n", line); }
		else { break; }
	}
	while (retries--);
	if (retries == 0) { afprintf(ferr, L"Max retries exceeded\n"); return; }
	afprintf(fout, L"Loading table from '"WSTR_FMT"'\n", line);
	if (load_table(fload, ferr, table, line)) {
		afprintf(fout, L"Loaded table\n");
	}
	else {
		afprintf(ferr, L"Cannot load table\n");
	}
	if (fload != NULL) fclose(fload);
}

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, "ru_RU.utf8");
	setlocale(LC_NUMERIC, "C"); // float dots
#ifdef _WIN32
	_setmode(_fileno(stdin), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stderr), _O_U16TEXT);
#else
	fwide(stdin, 1);
	fwide(stdout, 1);
	fwide(stderr, 1);
#endif
	// sort by column (ASC / DESC)
	FILE *fin = stdin; // no free
	FILE *fout = stdout; // no free
	FILE *ferr = stderr; // no free
#ifdef _MCS_VER
#else
#endif
	table_t *table = NULL;
	wchar_t line[MAX_LINE_SIZE] = {0};
	if (!(argc > 1 && strcmp(argv[1], "--no-menu") == 0)) print_menu(fout);
	int retries = 3;
	afprintf(fout, WSTR_FMT L"\n", DIGITS ALPH_EN ALPH_RU);
	while (1) {
		afprintf(fout, L"> ");
		if (fgetws(line, MAX_LINE_SIZE, fin) == NULL) {
			// don't think it's actually possible with stack-allocated `line`
			// edit: possible when redirecting stdin
			afprintf(ferr, L"Get line error (fgets returned NULL)\n");
			break;
		}
		else if (ferror(fin)) {
			afprintf(ferr, L"Everything is bad\n"); break;
		}
		else if (feof(fin)) {
			afprintf(ferr, L"EOF\n"); break;
		}
		else if (line[0] == L'\n') {continue;}
		else if (PROMPT(L"q") || PROMPT(L"quit") || PROMPT(L"exit")) {
			afprintf(fout, L"Quitting\n"); break;
		}
		else if (PROMPT(L"h") || PROMPT(L"help")) {
			print_menu(fout);
		}
		else if (PROMPT(L"f") || PROMPT(L"fill")) {
			fill_table(fin, fout, ferr, line, retries, &table);
		}
		else if (PROMPT(L"p") || PROMPT(L"print")) {
			print_table(fout, ferr, table, 0);
		}
		else if (PROMPT(L"a") || PROMPT(L"add")) {
			if (table == NULL) table = table_new(16);
			add_row(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT(L"d") || PROMPT(L"delete")) {
			delete_row(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT(L"w") || PROMPT(L"where")) {
			filter_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT(L"o") || PROMPT(L"order") || PROMPT(L"sort")) {
			sort_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT(L"s") || PROMPT(L"save") || PROMPT(L"export")) {
			export_table(fin, fout, ferr, table, line, retries);
		}
		else if (PROMPT(L"l") || PROMPT(L"load") || PROMPT(L"import")) {
			import_table(fin, fout, ferr, &table, line, retries);
		}
		else if (PROMPT(L"t_i")) {
			int64_t testi;
			afprintf(fout, L"Int: ");
			fgetws(line, MAX_LINE_SIZE, fin);
#ifdef _MSC_VER
			if (wparse_int(line, &testi)) afprintf(fout, L"%lld\n", testi);
#else
			if (wparse_int(line, &testi)) afprintf(fout, L"%zd\n", testi);
#endif
		}
		else if (PROMPT(L"t_f")) {
			double testf;
			afprintf(fout, L"Float: ");
			fgetws(line, MAX_LINE_SIZE, fin);
			if (wparse_float(line, &testf)) afprintf(fout, L"%f\n", testf);
		}
		else if (PROMPT(L"t_c")) {
			for (unsigned int i = 0; i < 100; i++) {
				afprintf(fout, L"\n");
			}
		}
		else if (PROMPT(L"t_q")) {
			// silent quit
			break;
		}
		else if (PROMPT(L"t_a")) {
			afprintf(fout, L": ");
			fgetws(line, MAX_LINE_SIZE, fin);
			for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != L'\n' && line[i] != L'\0'; i++) {
				afprintf(fout, L"[%4zu] %c - %d\n", i, line[i], line[i]);
			}
		}
		else {
			afprintf(fout, L"Unknown command: "WSTR_FMT, line);
		}
	}
	if (table != NULL) table_free(table);
	return EXIT_SUCCESS;
}
