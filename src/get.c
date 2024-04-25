#include <string.h>

#include "get.h"
#include "defs.h"
#include "parse.h"

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
#define ALPH_EN_UPP "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ALPH_EN ALPH_EN_LOW ALPH_EN_UPP
#define ALPH_RU_LOW "абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
#define ALPH_RU_UPP "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
#define ALPH_RU ALPH_RU_LOW ALPH_RU_UPP

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
			DIGITS ALPH_EN ALPH_RU, 16, out_result);
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

