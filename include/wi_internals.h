#ifndef WI_INTERNALS_HEADER_GUARD
#define WI_INTERNALS_HEADER_GUARD

#include "wi_data.h"

void restore_terminal(void);
void raw_terminal(void);
int input_function(void* args);
int render_function(void* args);

/* utility-functions, I didn't want to make an extra headerfile for this */

typedef struct {
	unsigned short width;
	unsigned short bytes;
} code_lengths;

/*
 * A function that takes in a pointer to a string, and returns how much visual
 * space (in characters) that "thing" takes up.
 *
 * Currently supports UTF8-encoded characters that take up 1 character,
 * and ansii escape codes that take up 0 characters ('\033[...m').
 *
 * No support yet for other zero-width characters or grapheme clusters.
 *
 * Returns the result as a `wi_position`, where the row is the amount of
 * visual space the characters takes, and the col is the amount of bytes the
 * character takes.
 */
code_lengths char_byte_size(const char*);

/*
 * A function that processes a string into lines of strings, and returns the
 * result as an array of structs containing each a line, line-length in bytes
 * and in visible characters (see `char_byte_size()`).
 *
 * A line is defined as a series of non-newlines, ended by a newline.
 * The newline itself is stripped.
 */
wi_content* split_lines(const char*);

/* Decrement index-pointer when on continuation byte until not anymore */
void skip_continuation_bytes_left(int*, const char*);

/* Increment index-pointer when on continuation byte until not anymore */
void skip_continuation_bytes_right(int*, const char*, const int max);

#endif	/* !WI_INTERNALS_HEADER_GUARD */
