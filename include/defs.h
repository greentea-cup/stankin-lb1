#ifndef DEFS_H
#define DEFS_H

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

// Note: both gcc and clang optimize away strlen calls on static string literals
// msvc does it only with /O2, so x64-debug may be a bit unoptimized
#define PROMPT(x) (strlen(line) == strlen(x "\n") && !strncmp(x "\n", line, strlen(x "\n")))
#define afprintf(stream, ...) { if (stream != NULL) fprintf(stream, __VA_ARGS__); }

#endif

