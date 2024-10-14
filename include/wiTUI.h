#pragma once

#include <stdbool.h>

typedef struct wi_position {
	int row;
	int col;
} wi_position;

typedef enum wi_modifier {
	CTRL, ALT, SHIFT, SUPER
} wi_modifier;

typedef struct wi_movement_keys {
	char left;
	char right;
	char up;
	char down;
	char quit;
	wi_modifier modifier_key;
} wi_movement_keys;

typedef struct wi_border {
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

typedef struct wi_session wi_session;
typedef struct wi_window wi_window;
typedef struct wi_window {
	int width;
	int height;
	/* (HEAP) */
	char* title;
	/* (HEAP) */
	char* footer;
	/* (HEAP) */
	char*** contents;
	wi_border border;
	bool wrapText;
	bool store_cursor_position;
	/* (HEAP) */
	wi_window** depending_windows;

	/* Only change this outside library code if you like debugging. */
    int _internal_amount_depending;
	/* Only change this outside library code if you like debugging. */
	int _internal_amount_rows;
	/* Only change this outside library code if you like debugging (HEAP). */
	int* _internal_amount_cols;
	/* Only change this outside library code if you like debugging. */
	int _internal_rendered_width;
	/* Only change this outside library code if you like debugging. */
	int _internal_rendered_height;
} wi_window;

typedef struct wi_session {
	/* (HEAP) */
	wi_window*** windows;
	bool full_screen;
	wi_position cursor_start;
	wi_movement_keys movement_keys;

	/* Only change this outside library code if you like debugging. */
	int _internal_amount_rows;
	/* Only change this outside library code if you like debugging (HEAP). */
	int* _internal_amount_cols;
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
 * If position.row > window.amount_rows, then the content will be placed on a 
 * new row directly below the previous last row.
 * If position.column > window[position.row].amount_columns, then the content
 * will be placed as last item in that row.
 *
 * To get empty contents, add empty strings as content.
 * To have the effect of falling back to previous content at some positions, add
 * NULL as content. See the README of this library for more info.
 *
 * @returns: updated window
 */
wi_window* wi_add_content_to_window(wi_window*, const char* content, const wi_position);

/* 
 * Set title of a window. 
 * This function assumes the previous title was set by malloc, and frees it. 
 *
 * @returns: updated window
 */
wi_window* wi_set_window_title(wi_window*, char* title);
