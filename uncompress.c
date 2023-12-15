#include "common.h"

/*
	Data structures and functions for the reverse code table, a trie that
	maps codes to sequences of bytes. The reverse code table is implemented
	as an array, with one slot for each code that identifies the parent code
	in the trie and the byte that connects parent and child. Codes are assigned
	to new byte sequences sequentially, with the empty byte sequence assigned
	to NULL_CODE.
*/

typedef struct reverse_code_table_entry {
	code_t parent;
	unsigned char byte;
} reverse_code_table_entry;

typedef struct reverse_code_table {
	reverse_code_table_entry* entries;
	size_t next_available_code;
	size_t capacity;
} reverse_code_table;

static inline void reverse_code_table_get(reverse_code_table* table,
		code_t code, code_t* parent, unsigned char* byte) {
	*byte = table->entries[code].byte;
	*parent = table->entries[code].parent;
}

static inline void reverse_code_table_add(reverse_code_table* table,
		code_t parent, unsigned char byte) {
	table->entries[table->next_available_code].parent = parent;
	table->entries[table->next_available_code].byte = byte;
	table->next_available_code++;
}

static inline bool reverse_code_table_out_of_range(reverse_code_table* table,
		code_t code) {
	return code >= table->next_available_code;
}

static inline void reverse_code_table_reset(reverse_code_table* table) {
	memset(table->entries, 0, table->capacity * sizeof(reverse_code_table_entry));

	table->next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		reverse_code_table_add(table, NULL_CODE, byte);
	}
}

static inline void reverse_code_table_init(reverse_code_table* table,
		size_t capacity) {
	table->capacity = capacity;
	table->entries = malloc(capacity * sizeof(reverse_code_table_entry));
	assert(table->entries != NULL);
	reverse_code_table_reset(table);
}

static inline void reverse_code_table_free(reverse_code_table* table) {
	free(table->entries);
}

/*
	Data structures and functions for an input buffer that reads from a file.
*/

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

/*
	Data structures and functions for a byte stack with fixed capacity,
	which also supports an expensive "push to bottom" operation. Note that
	this stack does no bounds checking (it is initialized in main to be
	big enough for any input).

	The stack also supports an fwrite_unlocked operation, which writes
	out the bytes in the stack, top-to-bottom, to a file, without clearing
	the stack. Like the underlying putc_unlocked, the function assumes
	that the caller holds a lock on the file.
*/

typedef struct stack {
	unsigned char* mem;
	size_t size;
	size_t capacity;
} stack;

static inline unsigned char stack_top(stack* s) {
	return s->mem[s->size - 1];
}

static inline void stack_fwrite_unlocked(stack* s, FILE* fd) {
	for (int pos = s->size - 1; pos >= 0; pos--) {
		int putc_unlocked_val = putc_unlocked(s->mem[pos], fd);
		assert(putc_unlocked_val != EOF);
	}
}

static inline void stack_push(stack* s, unsigned char c) {
	s->mem[s->size] = c;
	s->size++;
}

static inline void stack_push_bottom(stack* s, unsigned char c) {
	memmove(s->mem + 1, s->mem, s->size * sizeof(unsigned char));
	s->mem[0] = c;
	s->size++;
}

static inline void stack_clear(stack* s) {
	s->size = 0;
}

static inline void stack_init(stack* s, size_t capacity) {
	s->capacity = capacity;
	s->mem = malloc(capacity * sizeof(unsigned char));
	assert(s->mem != NULL);
	stack_clear(s);
}

static inline void stack_free(stack* s) {
	free(s->mem);
}

/*
	File decompressor implemented with the LZW algorithm.

	Usage: ./uncompress [input-path] [output-path]
*/
int main(int argc, char** argv) {
	check(argc == 3, "uncompress usage: ./uncompress [input-path] [output-path]");
	const char* input_path = argv[1];
	const char* output_path = argv[2];
	FILE* input_file = fopen(input_path, "r");
	check(input_file != NULL, "uncompress: error opening input file");
	FILE* output_file = fopen(output_path, "wx");
	check(output_file != NULL,
		"uncompress: error opening output file, it might exist already");
	flockfile(output_file);

	reverse_code_table table;
	reverse_code_table_init(&table, MAX_NUM_CODES);
	buffer b;
	buffer_init(&b);
	stack s;
	stack_init(&s, MAX_NUM_CODES);
	code_t code;
	code_t prev_code = NULL_CODE;
	while(buffer_read(&b, &code, input_file)) {
		if (code == NULL_CODE) {
			// NULL_CODE indicates state reset
			reverse_code_table_reset(&table);
			prev_code = NULL_CODE;
			stack_clear(&s);
		}
		else if (reverse_code_table_out_of_range(&table, code)) {
			// Special case (read a code that has no entry yet)
			unsigned char prev_top = stack_top(&s);
			reverse_code_table_add(&table, prev_code, prev_top);
			stack_push_bottom(&s, prev_top);
		}
		else {
			// Code that has entry: query reverse code table to get byte sequence,
			// and potentially add a new code to the table
			stack_clear(&s);
			code_t cur_code = code;
			while (cur_code != NULL_CODE) {
				unsigned char byte;
				reverse_code_table_get(&table, cur_code, &cur_code, &byte);
				stack_push(&s, byte);
			}
			if (prev_code != NULL_CODE) {
				reverse_code_table_add(&table, prev_code, stack_top(&s));
			}
		}
		stack_fwrite_unlocked(&s, output_file);
		prev_code = code;
	}
	assert(ferror(input_file) == 0);
	reverse_code_table_free(&table);
	stack_free(&s);

	funlockfile(output_file);
	int input_file_close_val = fclose(input_file);
	assert(input_file_close_val == 0);
	int output_file_close_val = fclose(output_file);
	assert(output_file_close_val == 0);
	exit(EXIT_SUCCESS);
}
