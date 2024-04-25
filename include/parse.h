#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^\+?[0-9]+\s*$/
 */
int wparse_uint(wchar_t *str, size_t *out_result);

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^(\+|-)?[0-9]+\s*$/
 */
int wparse_int(wchar_t *str, int64_t *out_result);

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format int [. [uint] ]
 */
int wparse_float(wchar_t *str, double *out_result);

#endif

