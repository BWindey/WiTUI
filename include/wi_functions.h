#ifndef WI_TUI_FUNCTIONS_HEADER_GUARD
#define WI_TUI_FUNCTIONS_HEADER_GUARD

#include "wi_data.h"
#include <stdbool.h>

/*
 * All functions are gathered here:
 * 	- create data-structures
 * 	- add things to data-structures
 * 	- display data-structures
 * 	- free data-structures
 * 	- handy functions for keymap
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
 *		- keybinds - empty
 *
 * When add_vim_keybindings is `true`, this function adds
 * 'hjkl' to move the cursor inside a window,
 * CTRL+'hjkl' to move focus between windows and 'q' to quit.
 *
 * See the library README.md for more details.
 *
 * @returns: created window
 */
wi_session* wi_make_session(bool add_vim_keybindings);


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
 *
 * @returns: the session with the update keymap
 */
wi_session* wi_add_keymap_to_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*callback)(const char, wi_session*)
);

/*
 * Remove a keymap from a session, where a keymap is defined by its key and
 * its modifier.
 * When the combination of key+modifier is more then once present in the
 * key mappings of the session, only the first will be removed.
 * When the combination is not present, nothing will happen. You just waisted
 * a few cpu cycles.
 *
 * @returns: the session with the update keymap
 */
wi_session* wi_pop_keymap_from_session(
	wi_session*, const char key, const wi_modifier
);

/*
 * Update a keymap from a session with a new callback-function.
 * A key mapping is defined by its key and its modifier.
 * Only the first combination of key + modifier and a not-NULL callback will
 * get the new callback function.
 *
 * @returns: the session with the update keymap
 */
wi_session* wi_update_keymap_from_session(
	wi_session*, const char key, const wi_modifier,
	void (*new_callback)(const char, wi_session*)
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
 */
void wi_show_session(wi_session*);

/*
 * Clear screen where the session was printed.
 * Only call this DIRECTLY after wi_show_session() has terminated.
 * Will use the '.rendered_width' and _height stored in the windows from
 * rendering them.
 * Assumes that the cursor is standing just under the rendered session.
 */
void wi_clear_screen_afterwards(wi_session*);


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
void wi_free_content(wi_content);



/* ---------------------------------------
 * Handy functions for making key mappings.
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
wi_content wi_get_current_window_content(const wi_window* window);

/*
 * Get the current cursor-position from a window.
 * The actual cursor-position is split over 2 internal variables, and can be
 * out of bounds (details...), so this is a utility function to easily get
 * what you want.
 */
wi_position wi_get_window_cursor_pos(const wi_window* window);


/*
 * Get 1 character from user.
 * When there is no character available to read, returns -1.
 * Assumes that the terminal is in raw, non-blocking mode, as set by
 * `wi_show_session()`.
 */
char wi_get_char(void);

/*
 * Stop rendering the session, will give control back to the caller of
 * `wi_show_session()`.
 * Does not guarantee that rendering has completely finished, so this is not
 * recommended to use inside a key map that prints something extra, as it could
 * happen that the print happens while the last frame of the session is still
 * being drawn. Use `wi_quit_rendering_and_wait()` to wait for drawing to be
 * complete.
 */
void wi_quit_rendering(const char, wi_session* session);

/*
 * Stop rendering the session, and wait until the render-thread and
 * input-thread have joined the thread running `wi_show_session()`.
 */
void wi_quit_rendering_and_wait(const char, wi_session* session);

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
wi_string_length wi_char_byte_size(const char*);

/*
 * A function like `strlen()`, but returns both byte- and visible length.
 * Does not include the ending nullbyte.
 */
wi_string_length wi_strlen(const char*);

/*
 * A function that returns a pointer to the currently focussed window.
 * Because the column-number can be out of bounds, I wanted to provide this
 * abstraction.
 */
wi_window* wi_get_focussed_window(wi_session*);

#endif /* !WI_TUI_FUNCTIONS_HEADER_GUARD */
