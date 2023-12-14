#include "common.h"

typedef struct code {
	code_t children[UCHAR_MAX + 1];
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
	code_t buf[BUFSIZE];
	size_t pos;
} buffer;

static inline void buffer_reset(buffer* b) {
	b->pos = 0;
}

static inline void buffer_write(buffer* b, code_t item, FILE* output_file, bool force_flush) {
	b->buf[b->pos] = item;
	(b->pos)++;
	if (b->pos == BUFSIZE || force_flush) {
		fwrite(b->buf, sizeof(code_t), b->pos, output_file);
		buffer_reset(b);
	}
}

int main(int argc, char** argv) {
	code* codes = malloc(MAX_NUM_CODES * sizeof(code));
	size_t next_available_code = reset_code_table(codes, MAX_NUM_CODES);

	const char* input_path = argv[1];
	const char* output_path = argv[2];

	FILE* input_file = fopen(input_path, "rb");
	FILE* output_file = fopen(output_path, "wb");
	flockfile(input_file);

	code_t code = NULL_CODE;
	int c;
	buffer b;
	buffer_reset(&b);
	while ((c = getc_unlocked(input_file)) != EOF) {
		code_t next_code = codes[code].children[c];
		if (next_code != NULL_CODE) {
			code = next_code;
		}
		else {
			buffer_write(&b, code, output_file, false);
			if (next_available_code == MAX_NUM_CODES) {
				buffer_write(&b, NULL_CODE, output_file, false);
				next_available_code = reset_code_table(codes, MAX_NUM_CODES);
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
