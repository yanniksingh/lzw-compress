#include "common.h"

typedef struct reverse_code_table_entry {
	code_t parent;
	unsigned char byte;
} reverse_code_table_entry;

typedef struct reverse_code_table {
	reverse_code_table_entry* entries;
	size_t next_available_code;
	size_t capacity;
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
	memset(table->entries, 0, table->capacity * sizeof(reverse_code_table_entry));

	table->next_available_code = FIRST_AVAILABLE_CODE;
	for (size_t byte = 0; byte <= UCHAR_MAX; byte++) {
		reverse_code_table_add(table, NULL_CODE, byte);
	}
}

static inline void reverse_code_table_init(reverse_code_table* table, size_t capacity) {
	table->capacity = capacity;
	table->entries = malloc(capacity * sizeof(reverse_code_table_entry));
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
		putc_unlocked(s->mem[pos], fd);
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
	stack_clear(s);
}

static inline void stack_free(stack* s) {
	free(s->mem);
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
	FILE* input_file = fopen(input_path, "r");
	FILE* output_file = fopen(output_path, "w");
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
			reverse_code_table_reset(&table);
			prev_code = NULL_CODE;
			stack_clear(&s);
		}
		else if (reverse_code_table_out_of_range(&table, code)) {
			unsigned char prev_top = stack_top(&s);
			reverse_code_table_add(&table, prev_code, prev_top);
			stack_push_bottom(&s, prev_top);
		}
		else {
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
	reverse_code_table_free(&table);
	stack_free(&s);

	funlockfile(output_file);
	fclose(output_file);
	fclose(input_file);
	return 0;
}
