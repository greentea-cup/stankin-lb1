#include <string.h>

#include "get.h"
#include "defs.h"

int parse_int0(line_t *line, int *out_result) {
	int res = 0;
	for (size_t i = line->start, e = line->end; i < e; i++) {
		res *= 10;
		char c = line->str[i];
		if (c < '0' || c > '9') return 0;
		res += c - '0';
	}
	*out_result = res;
	return 1;
}

int lparse_uint(line_t const *line, size_t *out_result) {
	if (line == NULL || out_result == NULL) return 0;
	if (ARG_LEN == 0) { *out_result = 0; return 1; }
	size_t start = line->start;
	char const *str = line->str + start;
	size_t len = line->end - start;
	if (str[0] == '+') { str++; len--; }
	size_t res = 0;
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		if (c >= '0' && c <= '9') {
			res = res * 10 + c - '0';
		}
		else {
			return 0;
		}
	}
	*out_result = res;
	return 1;
}

int lparse_int(line_t const *line, int64_t *out_result) {
	if (line == NULL || out_result == NULL) return 0;
	if (ARG_LEN == 0) { *out_result = 0; return 1; }
	size_t start = line->start;
	char const *str = line->str + start;
	size_t len = line->end - start;
	int neg = 0;
	if (str[0] == '+') { str++; neg = 0; len--; }
	else if (str[0] == '-') { str++; neg = 1; len--; }
	size_t res = 0;
	for (size_t i = 0; i < len; i++) {
		char c = str[i];
		if (c >= '0' && c <= '9') {
			res = res * 10 + c - '0';
		}
		else {
			return 0;
		}
	}
	*out_result = neg ? -(int64_t)res : (int64_t)res;
	return 1;
}

int lparse_float(line_t const *line, double *out_result) {
	if (line == NULL || out_result == NULL) return 0;
	if (ARG_LEN == 0) { *out_result = 0.0; return 1; }
	size_t start = line->start;
	char const *str = line->str + start;
	size_t len = line->end - start;
	int neg = 0;
	if (str[0] == '+') { str++; neg = 0; len--; }
	else if (str[0] == '-') { str++; neg = 1; len--; }
	size_t units = 0, parts = 0, div = 1;
	size_t i = 0;
	for (; i < len; i++) {
		char c = str[i];
		if (c >= '0' && c <= '9') {
			units = units * 10 + c - '0';
		}
		else if (c == '.') {
			break;
		}
		else {
			return 0;
		}
	}
	if (i < len && str[i] == '.') {
		for (i++; i < len; i++) {
			char c = str[i];
			if (c >= '0' && c <= '9') {
				parts = parts * 10 + c - '0';
				div *= 10;
			}
			else {
				return 0;
			}
		}
	}
	double res = units + (double)parts / div;
	*out_result = neg ? -res : res;
	return 1;
}

int lparse_bool(line_t const *line, bool *out_result) {
	if (line == NULL || out_result == NULL) return 0;
	if (ARG_LEN == 0) { *out_result = false; return 1; }
	size_t ifuint;
	if (lparse_uint(line, &ifuint)) { *out_result = (bool)ifuint; return 1; }
	size_t start = line->start, end = line->end;
	char const *str = line->str + start;
	size_t len = end - start;
	if (!strncmp("true", str, len) || !strncmp("TRUE", str, len)) { *out_result = true; return 1; }
	else if (!strncmp("false", str, len) || !strncmp("FALSE", str, len)) { *out_result = false; return 1; }
	else { return 0; }
}

int lparse_str(line_t const *line, size_t maxlen, char *out_result) {
	(void)line; (void)maxlen; (void)out_result;
	return 0;
}

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

int get_line(FILE *fin, line_t *line) {
	if (line == NULL || fin == NULL || ferror(fin) || feof(fin)) return 0;
	if (fgets(line->str, MAX_LINE_SIZE, fin) == NULL) return 0;
	line->start = line->end = 0;
	while (1) {
		char c = line->str[line->end];
		if (c == '\n' || c == '\0') break;
		line->end++;
	}
	line->str[line->end] = '\0';
	return 1;
}

int get_arg(FILE *fin, line_t *line) {
	if (fin == NULL || line == NULL) return 0;
	if (line->end >= MAX_LINE_SIZE - 1
		|| line->str[line->end + 1] == '\n'
		|| line->str[line->end + 1] == '\0') {
		if (ferror(fin) || feof(fin)) return 0;
		if (fgets(line->str, MAX_LINE_SIZE, fin) == NULL) return 0;
		line->start = 0;
	}
	else { line->start = line->next; }
	line->end = line->start;
	size_t offset = 0;
	char quoted = line->str[line->end], escaped = 0;
	if (quoted == '"' || quoted == '\'') { offset++; }
	else { quoted = 0; }
	char tmp[MAX_LINE_SIZE] = {0};
	size_t i = 0;
	while (1) {
		if (offset && line->end+offset < MAX_LINE_SIZE) {
			line->str[line->end] = line->str[line->end+offset];
		}
		char c = line->str[line->end];
		if ( (!quoted && !escaped && (c == ' ' || c == '\t'))
			|| c == '\n' || c == '\0') { break; }
		else if (!escaped && c == '\\') { escaped = 1; offset++; }
		else if (!quoted && (c == '"' || c == '\'')) { quoted = c; offset++; }
		else if (quoted && c == quoted) { quoted = 0; offset++; }
		else if (escaped) { escaped = 0; tmp[i++] = c; line->end++; }
		else { tmp[i++] = c; line->end++; }
	}
	for (size_t j = 0; j < i; j++) { line->str[line->start+j] = tmp[j]; }
	line->next = line->end+offset+1;
	return 1;
}

int get_uint(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt,
	char const *onerror,
	int interactive,
	size_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		get_arg(fin, line);
		if (lparse_uint(line, out_result) == 0) {
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

int get_int(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt,
	char const *onerror,
	int interactive,
	int64_t *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		get_arg(fin, line);
		if (lparse_int(line, out_result) == 0) {
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

int get_float(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt,
	char const *onerror, int interactive,
	double *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		get_arg(fin, line);
		if (lparse_float(line, out_result) == 0) {
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

int get_bool(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt,
	char const *onerror,
	int interactive, bool *out_result) {
	if (fin == NULL || out_result == NULL) return 0;
	do {
		if (prompt != NULL) afprintf(fout, "%s", prompt);
		get_arg(fin, line);
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
int get_str(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt,
	char const *onerror,
	int interactive, char const *whitelist, size_t maxlen, char *out_result) {
	int ii = interactive;
	do {
		afprintf(fout, "%s", prompt);
		get_line(fin, line);
		int good = 1;
		for (size_t i = 0; i < MAX_LINE_SIZE && line->str[i] != '\0' && line->str[i] != '\n'; i++) {
			if (i >= maxlen || strchr(whitelist, line->str[i]) == NULL) {
				good = 0;
				break;
			}
		}
		if (good) {
			size_t end = 0;
			while (end < MAX_LINE_SIZE - 1 && end < maxlen && line->str[end] != '\0' &&
				line->str[end] != '\n') end++;
			strncpy(out_result, line->str, end);
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

int get_id(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, size_t *out_result) {
	return get_uint(fin, fout, ferr, line, prompt != NULL ? onerror : "id[uint]: ",
			onerror != NULL ? onerror : "id: Uint expected\n", interactive,
			out_result);
}

int get_c1(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, int64_t *out_result) {
	return get_int(fin, fout, ferr, line, prompt != NULL ? prompt : "c1[int]: ",
			onerror != NULL ? onerror : "c1: Int expected\n", interactive, out_result);
}

int get_c2(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, double *out_result) {
	return get_float(fin, fout, ferr, line, prompt != NULL ? prompt : "c2[float]: ",
			onerror != NULL ? onerror : "c2: Float expected\n", interactive,
			out_result);
}

int get_c3(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, char *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : "c3[char 16]: ",
			onerror != NULL ? onerror : "c3: Max length: 16; Valid chars are 0-9 a-z A-Z\n", interactive,
			DIGITS ALPH_EN_LOW ALPH_EN_UPP, 16, out_result);
}

int get_c4(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, bool *out_result) {
	return get_bool(fin, fout, ferr, line, prompt != NULL ? prompt : "c4[bool]: ",
			onerror != NULL ? onerror : "c4: Valid options are: "
			"<blank = 0> 0 1 T[RUE] F[ALSE] t[rue] f[alse] ON OFF on off\n", interactive, out_result);
}

int get_c5(FILE *fin, FILE *fout, FILE *ferr, line_t *line, char const *prompt, char const *onerror,
	int interactive, char *out_result) {
	return get_str(fin, fout, ferr, line, prompt != NULL ? prompt : "c5[char 32]: ",
			onerror != NULL ? onerror : "c5: Max length: 31; Valid chars are 0-9 a-z A-Z \\s\n", interactive,
			DIGITS ALPH_EN_LOW ALPH_EN_UPP " ", 32, out_result);
}
