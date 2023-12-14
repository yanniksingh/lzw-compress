#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#define NULL_CODE 0
#define FIRST_AVAILABLE_CODE 1
#define BUFSIZE (1 << 10)
#define MAX_NUM_CODES (USHRT_MAX + 1)

typedef unsigned short code_t;
