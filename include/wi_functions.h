#ifndef WI_TUI_FUNCTIONS_HEADER_GUARD
#define WI_TUI_FUNCTIONS_HEADER_GUARD

#include "wi_data.h"

/*
 * All functions are gathered here:
 * 	- create data-structures
 * 	- add things to data-structures
 * 	- display data-structures
 * 	- free data-structures
 * 	- handy functions for keymaps
 */



/* ------------------------------
 * Create default data-structures
 * ------------------------------ */

/*
 * Create a default window with everything initialised.
 * Sets the following defaults:
 * 		- width = 10
 * 		- height = 10
 * 		- contents - empty
 * 		- border - empty title and footer, rounded borders
 * 		- wrap_text = false
 * 		- store_cursor_position = true
 * 		- depending_windows = NULL
 *
 * See the library README.md for more details.
 *
 * @returns: created window
 */
wi_window* wi_make_window(void);

/*
 * Create a default session.
 * Sets the following defaults:
 *		- windows - empty
 *		- start_clear_screen = false
 *		- focus_pos = { 0, 0 }
 *		- keymap - vim-keybindings
 *
 * See the library README.md for more details.
 *
 * @returns: created window
 */
wi_session* wi_make_session(void);


/* -----------------------------
 * Add stuff to data-structures.
 * ----------------------------- */

/*
 * Add window to an existing session at given row.
 * This function handles all the memory-management for you.
 * When the row is bigger then the current amount of rows +1,
 * the window will be placed on a new row below the current last row.
 * This prevents empty rows, which don't make sense.
 *
 * @returns: updated session
 */
wi_session* wi_add_window_to_session(wi_session*, wi_window*, int row);

/*
 * This function adds the 'depending' window as "dependency" to the 'parent'
 * window. This ensures that the 'depending' window will look to the cursor
 * position inside the 'parent' window to decide which content to show.
 *
 * When 'depending' already depended on another 'parent', that will be
 * overwritten with the new 'parent'.
 *
 * When 'depending' already was added as a "dependency" to the 'parent',
 * it will be twice in the 'parent'.depending list, making drawing a bit slower.
 */
void wi_bind_dependency(wi_window* parent, wi_window* depending);

/*
 * Add content-string to an existing window at the given position.
 * If position.row > window.amount_rows, then the NULL-contents will be placed
 * between empty rows (same for columns).
 *
 * To get empty contents, add empty strings as content.
 * To have the effect of falling back to previous content at some positions, add
 * NULL as content. See the README of this library for more info.
 *
 * @returns: updated window
 */
wi_window* wi_add_content_to_window(wi_window*, char* content, const wi_position);

/*
 * Add a new keymap to the session.
 * This function handles resource allocation for you.
 */
wi_session* wi_add_keymap_to_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*callback)(const char, wi_session*)
);



/* ----------------------------
 * Display the data-structures.
 * ---------------------------- */

/*
 * Print out one frame.
 *
 * @returns: height of printed frame.
 */
int wi_render_frame(wi_session*);

/*
 * Render a session to the screen, and take in user input.
 * Quits when `wi_quit_rendering()` is called on this session.
 *
 * @returns: last cursor position (which window + which coordinate).
 */
wi_result wi_show_session(wi_session*);



/* -------------------------
 * Free the data-structures.
 * ------------------------- */

/*
 * Free the session and all content inside it, so all the windows and their
 * contents too.
 *
 * If you want more fine-grained control, look at how this is implemented in
 * src/wi_tui.c and write your own implementation.
 *
 * @returns: void
 */
void wi_free_session(wi_session*);

/*
 * Free the window and all contents inside.
 *
 * Does NOT free depending windows, only the array of pointers to them.
 *
 * If you want more fine-grained control, look at how this is implemented in
 * src/wi_tui.c and write your own implementation.
 *
 * @returns: void
 */
void wi_free_window(wi_window*);

/*
 * Free the single content of a window. This is not the string you provided
 * to the library with 'wi_add_content_to_window()', but an internal
 * representation.
 */
void wi_free_content(wi_content*);



/* ---------------------------------------
 * Handy functions for making keymappings.
 * --------------------------------------- */

/*
 * Scroll up 1 line in the current content.
 * When that is not possible, do nothing.
 */
void wi_scroll_up(const char, wi_session* session);

/*
 * Scroll down 1 line in the current content.
 * When that is not possible, do nothing.
 */
void wi_scroll_down(const char, wi_session* session);

/*
 * Scroll left 1 character in the current content.
 * When that is not possible, do nothing.
 */
void wi_scroll_left(const char, wi_session* session);

/*
 * Scroll right 1 character in the current content.
 * When that is not possible, do nothing.
 */
void wi_scroll_right(const char, wi_session* session);

/*
 * Move current focus up by 1 window.
 * When that is not possible, do nothing.
 */
void wi_move_focus_up(const char, wi_session* session);

/*
 * Move current focus down by 1 window.
 * When that is not possible, do nothing.
 */
void wi_move_focus_down(const char, wi_session* session);

/*
 * Move current focus left by 1 window.
 * When that is not possible, do nothing.
 */
void wi_move_focus_left(const char, wi_session* session);

/*
 * Move current focus right by 1 window.
 * When that is not possible, do nothing.
 */
void wi_move_focus_right(const char, wi_session* session);

/*
 * Get the current content from a window by looking at its .depends_on.
 * For more info, see the README.
 *
 * @returns: wi_content struct with current window content
 */
wi_content* wi_get_current_window_content(const wi_window* window);

/*
 * Get 1 character from user.
 * When there is no character available to read, returns -1.
 * Assumes that the terminal is in raw, nonblocking mode, as set by
 * `wi_show_session()`.
 */
char wi_get_char(void);

/*
 * Stop rendering the session, will give control back to the caller of
 * wi_show_session().
 */
void wi_quit_rendering(const char, wi_session* session);

#endif /* !WI_TUI_FUNCTIONS_HEADER_GUARD */
