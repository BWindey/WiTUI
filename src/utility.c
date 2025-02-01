#include "wiAssert.h"
#include "wi_data.h"
#include "wi_internals.h"

#include <stdio.h>
#include <stdlib.h> 	/* malloc(), realloc() */

wi_string_length wi_char_byte_size(const char* c) {
	if (*c == '\0') {
		return (wi_string_length) { .width = 0, .bytes = 1 };
	} else if ((*c & 0xF0) == 0xF0) { 	/* 11110000 */
		return (wi_string_length) { .width = 1, .bytes = 4 };
	} else if ((*c & 0xE0) == 0xE0) { 	/* 11100000 */
		return (wi_string_length) { .width = 1, .bytes = 3 };
	} else if ((*c & 0xC0) == 0xC0) { 	/* 11000000 */
		return (wi_string_length) { .width = 1, .bytes = 2 };
	} else if (*c == '\033') {
		unsigned short bytes = 1;
		do {
			bytes++;
			c++;
		} while (*c != 'm' && *c != '\0');
		return (wi_string_length) { .width = 0, .bytes = bytes };
	} else {
		return (wi_string_length) { .width = 1, .bytes = 1 };
	}
}

wi_string_length wi_strlen(const char* c) {
	wi_string_length result = { 0, 0 };
	wi_string_length charlen;

	while (*c != '\0') {
		charlen = wi_char_byte_size(c);
		result.bytes += charlen.bytes;
		result.width += charlen.width;
		c += charlen.bytes;
	}

	return result;
}

#define INITIALISE_LINE_LIST_EL(i, char_p) \
	line_list[i].length.width = 0; \
	line_list[i].length.bytes = 0; \
	line_list[i].string = char_p;

wi_content split_lines(char* content) {
	wi_string_view original;
	original.string = content;
	original.length = (wi_string_length) { 0, 0 };

	int amount_lines = 0;
	int line_list_capacity = 10;
	wi_string_view* line_list = (wi_string_view*) malloc(
		line_list_capacity * sizeof(wi_string_view)
	);

	/* Initialise */
	INITIALISE_LINE_LIST_EL(0, original.string)

	int bytes = 0;
	int chars = 0;

	while (true) {
		if (content[bytes] == '\0') {
			amount_lines++;
			break;
		} else if (content[bytes] == '\n') {
			amount_lines++;

			/* Grow arrays if necessary */
			if (amount_lines == line_list_capacity) {
				line_list_capacity *= 2;
				line_list = (wi_string_view*) realloc(
					line_list, line_list_capacity * sizeof(wi_string_view)
				);
				wiAssert(
					line_list != NULL,
					"Failed to grow array when processing window content."
				);
			}

			bytes++;
			chars++;

			/* Initialise current line 1 character behind the newline */
			INITIALISE_LINE_LIST_EL(amount_lines, content + bytes)
		} else {
			wi_string_length char_len = wi_char_byte_size(content + bytes);
			bytes += char_len.bytes;
			chars += char_len.width;
			line_list[amount_lines].length.bytes += char_len.bytes;
			line_list[amount_lines].length.width += char_len.width;
		}
	}

	original.length.width = chars;
	original.length.bytes = bytes;

	return (wi_content) {
		.original = original,
		.line_list = line_list,
		.amount_lines = amount_lines
	};
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
