#pragma once

#include <stdatomic.h>
#include <stdbool.h> 	/* true, false */
#include <stddef.h>		/* size_t */
#include <termios.h>	/* struct termios */

/* Forward declarations */
typedef struct wi_session wi_session;
typedef struct wi_window wi_window;

/* Represents an array of lines that form the content of a window */
typedef struct {
	char** lines;
	size_t* line_lengths; /* Lengths excluding '\0' */
	int amount_lines;
	int internal_amount_lines;
} wi_content;

typedef struct wi_position {
	short row;
	short col;
} wi_position;

typedef enum wi_modifier {
	CTRL, ALT, SHIFT, NONE
} wi_modifier;

typedef enum wi_cursor_rendering {
	INVISIBLE, LINEBASED, POINTBASED
} wi_cursor_rendering;

typedef enum wi_info_alignment {
	LEFT, CENTER, RIGHT
} wi_info_alignment;

typedef struct wi_keymap {
	wi_modifier modifier;
	char key;
	void (*callback)(const char, wi_session*);
} wi_keymap;

/*
 * A struct to hold border-info.
 * If the border should not be displayed, set the elements to 0/NULL.
 */
typedef struct wi_border {
	char* title;
	char* footer;
	wi_info_alignment title_alignment;
	wi_info_alignment footer_alignment;

	char* corner_top_left;
	char* corner_top_right;
	char* corner_bottom_right;
	char* corner_bottom_left;
	char* side_left;
	char* side_right;
	char* side_top;
	char* side_bottom;
	char* focussed_colour;
	char* unfocussed_colour;
} wi_border;

typedef struct wi_window {
	int width;
	int height;

	/* (HEAP), but each individual content is on the stack */
	wi_content*** contents;
	wi_border border;

	bool wrapText;
	bool store_cursor_position;

	/* NOTE: This could be done in a seperate struct and a pointer,
	 * 	that will take up less space when it is not needed */
	wi_cursor_rendering cursor_rendering;
	/* (HEAP) */
	wi_window** depending_windows;
	wi_window* depends_on;

	/* Internal variables, do not change outside library-code,
	 * unless you really know what you're doing.
	 */
	struct {
		int amount_depending;

		int content_rows;
		int* content_cols;

		int rendered_width;
		int rendered_height;

		wi_position content_render_offset;
		wi_position visual_cursor_position;

		bool currently_focussed;
	} internal;
} wi_window;

typedef struct wi_session {
	/* (HEAP) */
	wi_window*** windows;
	bool full_screen;
	wi_position cursor_pos;
	wi_keymap* keymaps;

	atomic_bool keep_running;
	atomic_bool cursor_has_changed;

	struct {
		int amount_rows;
		int* amount_cols;
		int amount_keymaps;
		int keymap_array_size;
	} internal;
} wi_session;

typedef struct wi_result {
	wi_position last_window;
	wi_position last_cursor;
} wi_result;


/*
 * Free the session and all windows inside.
 *
 * If you want to reuse some windows or even window-contents,
 * you can not use this function.
 *
 * @returns: void
 */
void wi_free_session_completely(wi_session*);

/*
 * Free the window and all contents inside.
 *
 * Does NOT free depending windows, only the array of pointers to them.
 *
 * If you want to reuse some contents or borders,
 * you'll have to do some manual work.
 *
 * @returns: void
 */
void wi_free_window(wi_window*);

/*
 * Free the single content of a window.
 */
void wi_free_content(wi_content*);

/*
 * Print out one frame.
 *
 * @returns: height of printed frame.
 */
int wi_render_frame(wi_session*);
/*
 * Render a session to the screen, and take in user input.
 * Quits when the right key is pressed (see session.movement_keys).
 *
 * @returns: last cursor position (which window + which coordinate).
 */
wi_result wi_show_session(wi_session*);

/*
 * Create a window on the heap like the other functions expect.
 * Sets the following defaults:
 * 		- width = 10
 * 		- height = 10
 * 		- title = "Test window"
 * 		- footer = "q: quit"
 * 		- contents - empty
 * 		- border - unicode rounded borders and full-length lines
 * 		- wrap_text = true
 * 		- store_cursor_position = true
 * 		- depending_windows = NULL
 *
 * See the library README.md for more details.
 *
 * @returns: created window
 */
wi_window* wi_make_window(void);
/*
 * Create a session on the heap like the other functions expect.
 * Sets the following default:
 *		- windows - empty
 *		- full_screen = false
 *		- cursor_start = { 0, 0 }
 *		- movement_keys = { h, j ,k, l, CTRL }
 *
 * See the library README.md for more detaisl.
 *
 * @returns: created window
 */
wi_session* wi_make_session(void);

/*
 * Add window to an existing session at given row.
 * This function handles all the memory-management for you.
 * When the row is bigger then the current amount of rows +1,
 * the window will be placed on a new row below the current last row.
 *
 * @returns: updated session
 */
wi_session* wi_add_window_to_session(wi_session*, wi_window*, int row);
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
 * Set title of a window.
 * This function assumes the previous title was set by malloc, and frees it.
 *
 * @returns: updated window
 */
wi_window* wi_set_window_title(wi_window*, char* title);

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
 * This function waits until there is 1 character available.
 */
char wi_get_char(void);

/*
 * Stop rendering the session, will give control back to the caller of
 * wi_show_session().
 */
void wi_quit_rendering(const char, wi_session* session);

/*
 * Add a new keymap to the session.
 * This function handles resource allocation for you.
 */
wi_session* wi_add_keymap_to_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*callback)(const char, wi_session*)
);
