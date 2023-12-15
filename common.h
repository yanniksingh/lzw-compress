#ifndef COMPRESS_COMMON_H
#define COMPRESS_COMMON_H

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NULL_CODE 0
#define FIRST_AVAILABLE_CODE 1
#define BUFSIZE (1 << 10)
#define MAX_NUM_CODES (USHRT_MAX + 1)

typedef unsigned short code_t;

static inline void check(bool condition, const char* error_string) {
	if (!condition) {
		fprintf(stderr, "%s\n", error_string);
		exit(EXIT_FAILURE);
	}
}

#endif
