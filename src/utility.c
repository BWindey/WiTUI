#include "wi_internals.h"

#include <stdlib.h> 	/* malloc(), realloc() */
#include <string.h>		/* strcpy() */

unsigned short utf8_byte_size(char c) {
	if ((c & 0xF0) == 0xF0) { 			/* 11110000 */
		return 4;
	} else if ((c & 0xE0) == 0xE0) { 	/* 11100000 */
		return 3;
	} else if ((c & 0xC0) == 0xC0) { 	/* 11000000 */
		return 2;
	} else {
		return 1;
	}
}

wi_content* split_lines(const char* content) {
	int internal_amount_lines = 10;
	int amount_lines = 0;
	char** lines = (char**) malloc(internal_amount_lines * sizeof(char*));
	unsigned int* line_lengths_bytes = (unsigned int*) malloc(
		internal_amount_lines * sizeof(unsigned int*)
	);
	unsigned int* line_lengths_chars = (unsigned int*) malloc(
		internal_amount_lines * sizeof(unsigned int*)
	);

	int i = 0;
	int line_length_bytes = 0;
	int line_length_chars = 0;

	while (true) {
		if (content[i] == '\n' || content[i] == '\0') {
			/* Grow arrays if necessary */
			if (amount_lines == internal_amount_lines) {
				internal_amount_lines += 10;
				lines = (char**) realloc(
					lines, internal_amount_lines * sizeof(char*)
				);
				line_lengths_bytes = (unsigned int*) realloc(
					line_lengths_bytes, internal_amount_lines * sizeof(unsigned int)
				);
				line_lengths_chars = (unsigned int*) realloc(
					line_lengths_chars, internal_amount_lines * sizeof(unsigned int)
				);
			}

			/* Store info in the arrays */
			if (line_length_chars == 0) {
				line_lengths_bytes[amount_lines] = 1;
				line_lengths_chars[amount_lines] = 1;
				lines[amount_lines] = (char*) malloc(2 * sizeof(char));
				strcpy(lines[amount_lines], " ");
			} else {
				line_lengths_chars[amount_lines] = line_length_chars;
				line_lengths_bytes[amount_lines] = line_length_bytes;
				lines[amount_lines] = (char*) malloc(
					(line_length_bytes + 1) * sizeof(char)
				);
				memcpy(
					lines[amount_lines], 				/* Destination */
					content + i - line_length_bytes, 	/* Source */
					line_length_bytes					/* Size */
				);
				lines[amount_lines][line_length_bytes] = '\0';
			}

			amount_lines++;
			line_length_chars = 0;
			line_length_bytes = 0;
			if (content[i] == '\0') {
				break;
			}
			i++;
		} else {
			int codepoint_length = utf8_byte_size(content[i]);
			line_length_bytes += codepoint_length;
			i += codepoint_length;
			line_length_chars += 1;
		}
	}


	wi_content* split_contents = (wi_content*) malloc(sizeof(wi_content));
	*split_contents = (wi_content){
		.lines = lines,
		.line_lengths_bytes = line_lengths_bytes,
		.line_lengths_chars = line_lengths_chars,
		.amount_lines = amount_lines,
		.internal_amount_lines = internal_amount_lines
	};

	return split_contents;
}

void skip_continuation_bytes_left(int* p, const char* c) {
	while (*p > 0 && (c[*p] & 0xC0) == 0x80) {
		(*p)--;
	}
}

void skip_continuation_bytes_right(int* p, const char* c, const int max) {
	while (*p < max && (c[*p] & 0xC0) == 0x80) {
		(*p)++;
	}
}
