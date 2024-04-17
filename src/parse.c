#include <string.h>

#include "parse.h"

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

