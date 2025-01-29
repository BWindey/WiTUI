#ifndef WI_INTERNALS_HEADER_GUARD
#define WI_INTERNALS_HEADER_GUARD

#include "wi_data.h"

void restore_terminal(void);
void raw_terminal(void);
int input_function(void* args);
int render_function(void* args);

/* utility-functions, I didn't want to make an extra headerfile for this */

/*
 * A function that processes a string into lines of strings, and returns the
 * result as an array of structs containing each a line, line-length in bytes
 * and in visible characters (see `wi_char_byte_size()`).
 *
 * A line is defined as a series of non-newlines, ended by a newline.
 * The newline itself is stripped.
 */
wi_content split_lines(char*);

/* Decrement index-pointer when on continuation byte until not anymore */
void skip_continuation_bytes_left(int*, const char*);

/* Increment index-pointer when on continuation byte until not anymore */
void skip_continuation_bytes_right(int*, const char*, const int max);

#endif	/* !WI_INTERNALS_HEADER_GUARD */
