#ifndef WI_INTERNALS_HEADER_GUARD
#define WI_INTERNALS_HEADER_GUARD

#include "wi_data.h"

void restore_terminal(void);
void raw_terminal(void);
int input_function(void* args);
int render_function(void* args);

/* utility-functions, I didn't want to make an extra headerfile for this */
unsigned short utf8_byte_size(char);
wi_content* split_lines(const char*);

#endif	/* !WI_INTERNALS_HEADER_GUARD */
