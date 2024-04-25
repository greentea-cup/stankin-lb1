#include <string.h>

#include "parse.h"

int wparse_uint(wchar_t *str, size_t *out_result) {
	if (str == NULL || str[0] == L'\0' || out_result == NULL) return 0;
	if (str[0] == L'+') str++;
	size_t res = 0;
	while (*str != L'\0') {
		if (*str >= L'0' && *str <= L'9') {
			res = res * 10 + *str - L'0';
			str++;
		}
		else if (*str == L' ' || *str == L'\n' || *str == L'\t') { break; }
		else { return 0; }
	}
	*out_result = res;
	return 1;
}

int wparse_int(wchar_t *str, int64_t *out_result) {
	if (str == NULL || str[0] == L'\0' || out_result == NULL) return 0;
	int neg = 0;
	if (str[0] == L'+') str++;
	else if (str[0] == L'-') { neg = 1, str++; };
	int64_t res = 0;
	while (*str != L'\0') {
		if (*str >= L'0' && *str <= L'9') {
			res = res * 10 + *str - L'0';
			str++;
		}
		else if (*str == L' ' || *str == L'\n' || *str == L'\t') { break; }
		else { return 0; }
	}
	*out_result = neg ? -res : res;
	return 1;
}

int wparse_float(wchar_t *str, double *out_result) {
	if (str == NULL || str[0] == L'\0' || out_result == NULL) return 0;
	int neg = 0;
	if (str[0] == L'+') str++;
	else if (str[0] == L'-') { neg = 1; str++; }
	size_t full = 0, dec = 0, dv = 1;
	while (*str != L'\0') {
		if (*str >= L'0' && *str <= L'9') {
			full = full * 10 + *str - L'0';
			str++;
		}
		else if (*str == L'.') { break; }
		else if (*str == L' ' || *str == L'\n' || *str == L'\t') { break; }
		else { return 0; }
	}
	if (*str == L'.') {
		str++;
		while (*str != L'\0') {
			if (*str >= L'0' && *str <= L'9') {
				dv *= 10;
				dec = dec * 10 + *str - L'0';
				str++;
			}
			else if (*str == L' ' || *str == L'\n' || *str == L'\t') { break; }
			else { return 0; }
		}
	}
	double res = full + (double)dec / dv;
	*out_result = neg ? -res : res;
	return 1;
}

