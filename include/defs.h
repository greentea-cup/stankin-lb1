#ifndef DEFS_H
#define DEFS_H

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

// Note: msvc needs /Oi flag (request to generate intrinsics)
// for this to inline strlen calls on string literals
#define PROMPT(x) (strlen(line) == (strlen(x) + 1) && !strncmp(x, line, strlen(x)) && line[strlen(x)] == '\n')
#define afprintf(stream, ...) { if (stream != NULL) fprintf(stream, __VA_ARGS__); }

#endif

