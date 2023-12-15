#include "common.h"

typedef struct code_table_entry {
	code_t children[UCHAR_MAX + 1];
} code_table_entry;

typedef struct code_table {
	code_table_entry* entries;
	size_t next_available_code;
	size_t capacity;
} code_table;

static inline code_t code_table_get(code_table* table, code_t code,
		unsigned char byte) {
	return table->entries[code].children[byte];
}

static inline void code_table_add(code_table* table, code_t code,
		unsigned char byte) {
	table->entries[code].children[byte] = table->next_available_code;
	table->next_available_code++;
}

static inline bool code_table_full(code_table* table) {
	return table->next_available_code == table->capacity;
}

static inline void code_table_reset(code_table* table) {
	memset(table->entries, 0, table->capacity * sizeof(code_table_entry));

	table->next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		code_table_add(table, NULL_CODE, byte);
	}
}

static inline void code_table_init(code_table* table, size_t capacity) {
	table->capacity = capacity;
	table->entries = malloc(capacity * sizeof(code_table_entry));
	assert(table->entries != NULL);
	code_table_reset(table);
}

static inline void code_table_free(code_table* table) {
	free(table->entries);
}

typedef struct buffer {
	code_t buf[BUFSIZE];
	size_t pos;
} buffer;

static inline void buffer_init(buffer* b) {
	b->pos = 0;
}

static inline void buffer_write(buffer* b, code_t item, FILE* output_file,
		bool force_flush) {
	b->buf[b->pos] = item;
	(b->pos)++;
	if (b->pos == BUFSIZE || force_flush) {
		size_t codes_written = fwrite(b->buf, sizeof(code_t), b->pos, output_file);
		assert(codes_written == b->pos);
		b->pos = 0;
	}
}

int main(int argc, char** argv) {
	check(argc == 3, "compress usage: ./compress [input-path] [output-path]");
	const char* input_path = argv[1];
	const char* output_path = argv[2];

	FILE* input_file = fopen(input_path, "r");
	check(input_file != NULL, "compress: error opening input file");
	FILE* output_file = fopen(output_path, "wx");
	check(output_file != NULL,
		"compress: error opening output file, it might exist already");
	flockfile(input_file);

	code_table table;
	code_table_init(&table, MAX_NUM_CODES);
	buffer b;
	buffer_init(&b);
	code_t code = NULL_CODE;
	int byte;
	while ((byte = getc_unlocked(input_file)) != EOF) {
		code_t next_code = code_table_get(&table, code, byte);
		if (next_code != NULL_CODE) {
			code = next_code;
		}
		else {
			buffer_write(&b, code, output_file, false);
			if (code_table_full(&table)) {
				buffer_write(&b, NULL_CODE, output_file, false);
				code_table_reset(&table);
			}
			else {
				code_table_add(&table, code, byte);
			}
			code = code_table_get(&table, NULL_CODE, byte);
		}
	}
	assert(ferror(input_file) == 0);
	buffer_write(&b, code, output_file, true);
	code_table_free(&table);

	funlockfile(input_file);
	int input_file_close_val = fclose(input_file);
	assert(input_file_close_val == 0);
	int output_file_close_val = fclose(output_file);
	assert(output_file_close_val == 0);
	exit(EXIT_SUCCESS);
}
