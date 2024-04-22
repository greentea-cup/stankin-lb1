#ifndef GET_H
#define GET_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "defs.h"

typedef struct {
	size_t start;
	size_t end;
	size_t next;
	char str[MAX_LINE_SIZE];
} line_t;
#define LINE_FMT "%.*s"
#define LINE_LEN(x) ((x)->end - (x)->start)
#define LINE_STR(x) ((x)->str + (x)->start)
#define LINE_ARG(x) (int)LINE_LEN(x), LINE_STR(x)
#define ARG_LEN LINE_LEN(line)
// Note: msvc needs /Oi flag (request to generate intrinsics)
// for this to inline strlen calls on string literals
#define PROMPT(x) ( (ARG_LEN == strlen(x)) && (strncmp(line->str + line->start, x, ARG_LEN) == 0) )

int get_line(FILE *fin, line_t *line);

int get_arg(FILE *fin, line_t *line);

int lparse_uint(line_t const *line, size_t *out_result);

int lparse_int(line_t const *line, int64_t *out_result);

int lparse_float(line_t const *line, double *out_result);

int lparse_bool(line_t const *line, bool *out_result);

int lparse_str(line_t const *line, size_t maxlen, char *out_result);

// TODO:
// add lparse_* and implementations
// (do not remove regular parse_*)
// move from parse(line) to lparse(line) in main and get_*
// rename get_arg to next_arg or larg
// support for bash-like string handling:
// - no spaces can be entered without ticks
// - with spaces needs either "" or ''
// (special symbols are not allowed so no need to handle them for now)

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

/*
 * interactive = 1 if interactive input
 * returns 1 on success, 0 on failure
 */
int get_uint(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, size_t *out_result);

int get_int(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, int64_t *out_result);

int get_float(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, double *out_result);

int get_bool(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, bool *out_result);

/*
 * returns 1 on success, 0 on failure
 * retries interactive times
 * out_result should have capacity of maxlen + 1 characters (maxlen in chars + '\0')
 */
int get_str(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, char const *whitelist, size_t maxlen,
	char *out_result);

int get_id(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror,
	int interactive, size_t *out_result);

int get_c1(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, int64_t *out_result);

int get_c2(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, double *out_result);

int get_c3(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, char *out_result);

int get_c4(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, bool *out_result);

int get_c5(FILE *fin, FILE *fout, FILE *ferr, line_t *line,
	char const *prompt, char const *onerror, int interactive, char *out_result);

#endif
