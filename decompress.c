#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define NULL_CODE 0
#define FIRST_AVAILABLE_CODE 1

typedef struct reverse_code {
	unsigned short parent;
	unsigned char symbol;
} reverse_code;

static inline size_t reset_reverse_code_table(reverse_code* reverse_codes, size_t reverse_codes_size) {
	memset(reverse_codes, 0, reverse_codes_size * sizeof(reverse_code));

	size_t next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		reverse_codes[next_available_code].parent = NULL_CODE;
		reverse_codes[next_available_code].symbol = (unsigned char)byte;
		next_available_code++;
	}
	return next_available_code;
}

int main(int argc, char** argv) {
	size_t reverse_codes_size = USHRT_MAX + 1;
	reverse_code* reverse_codes = malloc(reverse_codes_size * sizeof(reverse_code));
	size_t next_available_code = reset_reverse_code_table(reverse_codes, reverse_codes_size);

	const char* input_path = argv[1];
	const char* output_path = argv[2];

	FILE* input_file = fopen(input_path, "rb");
	FILE* output_file = fopen(output_path, "wb");
	flockfile(output_file);

	unsigned short code;
	unsigned short prev_code = NULL_CODE;
	unsigned char* symbols = malloc(USHRT_MAX * sizeof(unsigned char));
	int symbols_top = 0;
	while(fread(&code, sizeof(unsigned short), 1, input_file) != 0) {
		if (code == NULL_CODE) {
			next_available_code = reset_reverse_code_table(reverse_codes, reverse_codes_size);
			prev_code = NULL_CODE;
			symbols_top = 0;
		}
		else if (code >= next_available_code) {
			unsigned char first_prev_symbol = symbols[symbols_top - 1];
			reverse_codes[next_available_code].parent = prev_code;
			reverse_codes[next_available_code].symbol = first_prev_symbol;
			next_available_code++;

			memmove(symbols + 1, symbols, symbols_top * sizeof(unsigned char));
			symbols[0] = first_prev_symbol;
			symbols_top++;
		}
		else {
			unsigned short cur_code = code;
			symbols_top = 0;
			while (cur_code != NULL_CODE) {
				symbols[symbols_top] = reverse_codes[cur_code].symbol;
				symbols_top++;
				cur_code = reverse_codes[cur_code].parent;
			}
			if (prev_code != NULL_CODE) {
				reverse_codes[next_available_code].parent = prev_code;
				reverse_codes[next_available_code].symbol = symbols[symbols_top - 1];
				next_available_code++;
			}
		}
		prev_code = code;
		for (int pos = symbols_top - 1; pos >= 0; pos--) {
			putc_unlocked(symbols[pos], output_file);
		}
	}

	funlockfile(output_file);
	fclose(output_file);
	fclose(input_file);
	return 0;
}