#include <string.h>

#include "get.h"
#include "defs.h"
#include "parse.h"

int get_uint(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive,
	size_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, WSTR_FMT, prompt);
		fgetws(line, MAX_LINE_SIZE, fin);
		if (wparse_uint(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, WSTR_FMT, onerror);
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

int get_int(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive,
	int64_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, WSTR_FMT, prompt);
		fgetws(line, MAX_LINE_SIZE, fin);
		if (wparse_int(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, WSTR_FMT, onerror);
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

int get_float(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror, int interactive,
	double *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, WSTR_FMT, prompt);
		fgetws(line, MAX_LINE_SIZE, fin);
		if (wparse_float(line, out_result) == 0) {
			if (interactive) {
				if (onerror != NULL) afprintf(ferr, WSTR_FMT, onerror);
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

int get_bool(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, bool *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, WSTR_FMT, prompt);
		fgetws(line, MAX_LINE_SIZE, fin);
		if (PROMPT(L"") || PROMPT(L"0") || PROMPT(L"F") || PROMPT(L"f")
			|| PROMPT(L"OFF") || PROMPT(L"off")
			|| PROMPT(L"FALSE") || PROMPT(L"false")) {
			*out_result = 0;
			break;
		}
		else if (PROMPT(L"1") || PROMPT(L"T") || PROMPT(L"t")
			|| PROMPT(L"ON") || PROMPT(L"on")
			|| PROMPT(L"TRUE") || PROMPT(L"true")) {
			*out_result = 1;
			break;
		}
		else if (interactive) {
			afprintf(ferr, WSTR_FMT, onerror);
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
 * out_result should have capacity of maxlen + 1 characters (maxlen in chars + L'\0')
 */
int get_str(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, wchar_t const *whitelist, size_t maxlen, wchar_t *out_result) {
	int ii = interactive;
	do {
		afprintf(fout, WSTR_FMT, prompt);
		fgetws(line, MAX_LINE_SIZE, fin);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line[i] != L'\0' && line[i] != L'\n'; i++) {
			if (i >= maxlen || wcschr(whitelist, line[i]) == NULL) {
				good = 0;
				break;
			}
		}
		if (good) {
			size_t end = 0;
			while (end < MAX_LINE_SIZE - 1 && end < maxlen && line[end] != L'\0' && line[end] != L'\n') end++;
			wcsncpy(out_result, line, end);
			out_result[end] = L'\0';
			return 1;
		}
		else if (interactive) {
			afprintf(ferr, WSTR_FMT, onerror);
		}
		else {
			return 0;
		}
	}
	while (interactive--);
	if (ii) {
		afprintf(ferr, L"Max retries exceeded\n");
		afprintf(fout, L"Cancelled\n");
	}
	return 0;
}

int get_id(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, size_t *out_result) {
	return get_uint(fin, fout, ferr, line, prompt != NULL ? onerror : L"id[uint]: ",
			onerror != NULL ? onerror : L"id: Uint expected\n", interactive,
			out_result);
}

int get_c1(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, int64_t *out_result) {
	return get_int(fin, fout, ferr, line, prompt != NULL ? prompt : L"c1[int]: ",
			onerror != NULL ? onerror : L"c1: Int expected\n", interactive, out_result);
}

int get_c2(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, double *out_result) {
	return get_float(fin, fout, ferr, line, prompt != NULL ? prompt : L"c2[float]: ",
			onerror != NULL ? onerror : L"c2: Float expected\n", interactive,
			out_result);
}

int get_c3(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, wchar_t *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : L"c3[wchar_t 16]: ",
			onerror != NULL ? onerror : L"c3: Max length: 16; Valid chars are 0-9 a-z A-Z\n", interactive,
			DIGITS ALPH_EN ALPH_RU, 16, out_result);
}

int get_c4(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, bool *out_result) {
	return get_bool(fin, fout, ferr, line, prompt != NULL ? prompt : L"c4[bool]: ",
			onerror != NULL ? onerror : L"c4: Valid options are: "
			L"<blank = 0> 0 1 T[RUE] F[ALSE] t[rue] f[alse] ON OFF on off\n", interactive, out_result);
}

int get_c5(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line, wchar_t const *prompt,
	wchar_t const *onerror,
	int interactive, wchar_t *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : L"c5[wchar_t 32]: ",
			onerror != NULL ? onerror : L"c5: Max length: 31; Valid chars are 0-9 a-z A-Z \\s\n", interactive,
			DIGITS ALPH_EN_LOW ALPH_EN_UPP L" ", 32, out_result);
}
