#ifndef WI_INTERNALS_HEADER_GUARD
#define WI_INTERNALS_HEADER_GUARD

void restore_terminal(void);
void raw_terminal(void);
char get_char(void);
int input_function(void* args);
int render_function(void* args);

#endif	/* !WI_INTERNALS_HEADER_GUARD */
