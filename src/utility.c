#include "wiAssert.h"
#include "wi_data.h"
#include "wi_internals.h"
#include "wi_functions.h"

#include <stdio.h>
#include <stdlib.h> 	/* malloc(), realloc() */

#define ADD_STR_LEN(X, Y) \
	X.width += (Y).width; \
	X.bytes += (Y).bytes;

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
		ADD_STR_LEN(result, charlen);
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
			ADD_STR_LEN(line_list[amount_lines].length, char_len);
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

bool can_break(char string) {
	switch (string) {
		case ' ':
		case '-':
			return true;
			break;

		case '\t':
			wiAssert(
				string != '\t',
				"Tabs are not allowed inside WiTUI due to alignment issues."
				"Use explicit spaces instead."
			);
	}
	return false;
}

wi_string_view calculate_next_line(char* content, int cols, int* bytes) {
	wi_string_view line = { .string = content };
	wi_string_length length = { 0, 0 };
	wi_string_length forward = { 0, 0 };

	while (forward.width < (unsigned) cols) {
		if (content[forward.bytes] == '\0' || content[forward.bytes] == '\n') {
			*bytes += 1;
			length = forward;
			break;
		} else if (can_break(content[forward.bytes])) {
			length = forward;
			if (length.width + 1 < (unsigned) cols) {
				length.width += 1;
				length.bytes += 1;
				forward.width += 1;
				forward.bytes += 1;
			}
		}
		wi_string_length cpl = wi_char_byte_size(content + forward.bytes);
		ADD_STR_LEN(forward, cpl);
	}

	if (length.bytes == 0) {
		length = forward;
	}

	line.length = length;
	*bytes += length.bytes;

	return line;
}

wi_content split_lines_wrapped(char* content, int cols) {
	int amount_lines = 0;
	int line_list_capacity = 10;
	wi_string_view* line_list = (wi_string_view*) malloc(
		line_list_capacity * sizeof(wi_string_view)
	);

	int bytes = 0;

	while (content[bytes] != '\0' && content[bytes + 1] != '\0') {
		line_list[amount_lines] = calculate_next_line(content + bytes, cols, &bytes);

		amount_lines++;
		if (amount_lines >= line_list_capacity) {
			line_list = (wi_string_view*) realloc(
				line_list, line_list_capacity * 2 * sizeof(wi_string_view)
			);
			wiAssert(line_list != NULL, "Failed to reallocate line_list");
			line_list_capacity *= 2;
		}
	}

	/* NOTE: I'm not keeping track of size here, as I don't think I need it? */
	wi_string_view original = {
		.string = content
	};

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

#undef ADD_STR_LEN
#undef INITIALISE_LINE_LIST_EL
