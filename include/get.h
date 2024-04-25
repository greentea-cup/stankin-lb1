#ifndef GET_H
#define GET_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <wchar.h>

/*
 * interactive = 1 if interactive input
 * returns 1 on success, 0 on failure
 */
int get_uint(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, size_t *out_result);

int get_int(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, int64_t *out_result);

int get_float(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, double *out_result);

int get_bool(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, bool *out_result);

/*
 * returns 1 on success, 0 on failure
 * retries interactive times
 * out_result should have capacity of maxlen + 1 characters (maxlen in chars + '\0')
 */
int get_str(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, wchar_t const *whitelist,
	size_t maxlen,
	wchar_t *out_result);

int get_id(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror,
	int interactive, size_t *out_result);

int get_c1(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, int64_t *out_result);

int get_c2(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, double *out_result);

int get_c3(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, wchar_t *out_result);

int get_c4(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, bool *out_result);

int get_c5(FILE *fin, FILE *fout, FILE *ferr, wchar_t *line,
	wchar_t const *prompt, wchar_t const *onerror, int interactive, wchar_t *out_result);

#endif
