#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>

#define NULL_CODE 0
#define FIRST_AVAILABLE_CODE 1
#define BUFSIZE (1 << 10)

typedef struct code {
	unsigned short children[UCHAR_MAX + 1];
} code;

static inline size_t reset_code_table(code* codes, size_t codes_size) {
	memset(codes, 0, codes_size * sizeof(code));

	size_t next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		codes[NULL_CODE].children[byte] = next_available_code;
		next_available_code++;
	}
	return next_available_code;
}

typedef struct buffer {
	unsigned short buf[BUFSIZE];
	size_t pos;
} buffer;

static inline void buffer_reset(buffer* b) {
	b->pos = 0;
}

static inline void buffer_write(buffer* b, unsigned short item, FILE* output_file, bool force_flush) {
	b->buf[b->pos] = item;
	(b->pos)++;
	if (b->pos == BUFSIZE || force_flush) {
		fwrite(b->buf, sizeof(unsigned short), b->pos, output_file);
		buffer_reset(b);
	}
}

int main(int argc, char** argv) {
	size_t codes_size = USHRT_MAX + 1;
	code* codes = malloc(codes_size * sizeof(code));
	size_t next_available_code = reset_code_table(codes, codes_size);

	const char* input_path = argv[1];
	const char* output_path = argv[2];

	FILE* input_file = fopen(input_path, "rb");
	FILE* output_file = fopen(output_path, "wb");
	flockfile(input_file);

	unsigned short code = NULL_CODE;
	int c;
	buffer b;
	buffer_reset(&b);
	while ((c = getc_unlocked(input_file)) != EOF) {
		unsigned short next_code = codes[code].children[c];
		if (next_code != NULL_CODE) {
			code = next_code;
		}
		else {
			buffer_write(&b, code, output_file, false);
			if (next_available_code == codes_size) {
				buffer_write(&b, NULL_CODE, output_file, false);
				next_available_code = reset_code_table(codes, codes_size);
			}
			else {
				codes[code].children[c] = next_available_code;
				next_available_code++;
			}
			code = codes[NULL_CODE].children[c];
		}
	}
	buffer_write(&b, code, output_file, true);

	funlockfile(input_file);
	fclose(input_file);
	fclose(output_file);
	return 0;
}
