#ifndef DEFS_H
#define DEFS_H

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

#ifndef MAX_FILE_PATH
#define MAX_FILE_PATH 1024
#endif

// Note: msvc needs /Oi flag (request to generate intrinsics)
// for this to inline strlen calls on string literals
#define PROMPT(wx) (wcslen(line) == (wcslen(wx) + 1) && !wcsncmp(wx, line, wcslen(wx)) && line[wcslen(wx)] == '\n')
#define afprintf(stream, ...) { if (stream != NULL) fwprintf(stream, __VA_ARGS__); }
#ifdef _MSC_VER
#define WSTR_FMT L"%ls"
#define WFOPEN_ARG L", ccs=UTF-8"
#else
#define WSTR_FMT L"%ls"
#define WFOPEN_ARG L""
#endif

#define DIGITS L"0123456789"
#define PUNCTS L".,;:?!@#()$%^&*`~'\"<>/\\[]{}|+-=_№"
#define ALPH_EN_LOW L"abcdefghijklmnopqrstuvwxyz"
#define ALPH_EN_UPP L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ALPH_EN ALPH_EN_LOW ALPH_EN_UPP
#define ALPH_RU_LOW L"абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
#define ALPH_RU_UPP L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
#define ALPH_RU ALPH_RU_LOW ALPH_RU_UPP

#endif

