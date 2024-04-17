#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>
#include <stdint.h>

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^\+?[0-9]+\s*$/
 */
int parse_uint(char *str, size_t *out_result);

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format /^(\+|-)?[0-9]+\s*$/
 */
int parse_int(char *str, int64_t *out_result);

/*
 * returns 1 on success and result in out_result, 0 on failure
 * success if str is in format int [. [uint] ]
 */
int parse_float(char *str, double *out_result);

#endif

