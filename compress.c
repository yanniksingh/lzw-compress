#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

const unsigned short NULL_CODE = 0;
#define FIRST_AVAILABLE_CODE 1

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
	while ((c = getc_unlocked(input_file)) != EOF) {
		unsigned short next_code = codes[code].children[c];
		if (next_code != NULL_CODE) {
			code = next_code;
		}
		else {
			fwrite(&code, sizeof(unsigned short), 1, output_file);
			if (next_available_code == codes_size) {
				fwrite(&NULL_CODE, sizeof(unsigned short), 1, output_file);
				next_available_code = reset_code_table(codes, codes_size);
			}
			else {
				codes[code].children[c] = next_available_code;
				next_available_code++;
			}
			code = codes[NULL_CODE].children[c];
		}
	}
	fwrite(&code, sizeof(unsigned short), 1, output_file);

	funlockfile(input_file);
	fclose(input_file);
	fclose(output_file);
	return 0;
}
