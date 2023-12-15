#include "common.h"

typedef struct reverse_code_table_entry {
	code_t parent;
	unsigned char byte;
} reverse_code_table_entry;

typedef struct reverse_code_table {
	reverse_code_table_entry* entries;
	size_t next_available_code;
	size_t size;
} reverse_code_table;

static inline void reverse_code_table_get(reverse_code_table* table, code_t code, code_t* parent, unsigned char* byte) {
	*byte = table->entries[code].byte;
	*parent = table->entries[code].parent;
}

static inline void reverse_code_table_add(reverse_code_table* table, code_t parent, unsigned char byte) {
	table->entries[table->next_available_code].parent = parent;
	table->entries[table->next_available_code].byte = byte;
	table->next_available_code++;
}

static inline bool reverse_code_table_out_of_range(reverse_code_table* table, code_t code) {
	return code >= table->next_available_code;
}

static inline void reverse_code_table_reset(reverse_code_table* table) {
	memset(table->entries, 0, table->size * sizeof(reverse_code_table_entry));

	table->next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		reverse_code_table_add(table, NULL_CODE, byte);
	}
}

static inline void reverse_code_table_init(reverse_code_table* table, size_t size) {
	table->size = size;
	table->entries = malloc(size * sizeof(reverse_code_table_entry));
	reverse_code_table_reset(table);
}

static inline void reverse_code_table_free(reverse_code_table* table) {
	free(table->entries);
}

typedef struct buffer {
	code_t buf[BUFSIZE];
	size_t pos;
	size_t end_pos;
} buffer;

static inline void buffer_init(buffer* b) {
	b->pos = 0;
	b->end_pos = 0;
}

static inline size_t buffer_read(buffer* b, code_t* item, FILE* input_file) {
	if (b->pos == b->end_pos) {
		b->pos = 0;
		b->end_pos = fread(b->buf, sizeof(code_t), BUFSIZE, input_file);
		if (b->end_pos == 0) return false;
	}
	*item = b->buf[b->pos];
	(b->pos)++;
	return true;
}

int main(int argc, char** argv) {
	const char* input_path = argv[1];
	const char* output_path = argv[2];

	FILE* input_file = fopen(input_path, "rb");
	FILE* output_file = fopen(output_path, "wb");
	flockfile(output_file);

	reverse_code_table table;
	reverse_code_table_init(&table, MAX_NUM_CODES);
	buffer b;
	buffer_init(&b);
	unsigned char* symbols = malloc(MAX_NUM_CODES * sizeof(unsigned char));
	int symbols_top = 0;
	code_t code;
	code_t prev_code = NULL_CODE;
	while(buffer_read(&b, &code, input_file)) {
		if (code == NULL_CODE) {
			reverse_code_table_reset(&table);
			prev_code = NULL_CODE;
			symbols_top = 0;
		}
		else if (reverse_code_table_out_of_range(&table, code)) {
			unsigned char first_prev_symbol = symbols[symbols_top - 1];
			reverse_code_table_add(&table, prev_code, first_prev_symbol);

			memmove(symbols + 1, symbols, symbols_top * sizeof(unsigned char));
			symbols[0] = first_prev_symbol;
			symbols_top++;
		}
		else {
			code_t cur_code = code;
			symbols_top = 0;
			while (cur_code != NULL_CODE) {
				unsigned char byte;
				reverse_code_table_get(&table, cur_code, &cur_code, &byte);
				symbols[symbols_top] = byte;
				symbols_top++;
			}
			if (prev_code != NULL_CODE) {
				reverse_code_table_add(&table, prev_code, symbols[symbols_top - 1]);
			}
		}
		prev_code = code;
		for (int pos = symbols_top - 1; pos >= 0; pos--) {
			putc_unlocked(symbols[pos], output_file);
		}
	}
	reverse_code_table_free(&table);

	funlockfile(output_file);
	fclose(output_file);
	fclose(input_file);
	return 0;
}
