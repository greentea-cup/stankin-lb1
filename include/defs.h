#ifndef DEFS_H
#define DEFS_H

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024
#endif

#define afprintf(stream, ...) { if (stream != NULL) fprintf(stream, __VA_ARGS__); }

#endif

