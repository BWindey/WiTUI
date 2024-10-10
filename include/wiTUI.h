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


void wi_free_session_completely(wi_session*);
void wi_free_window(wi_window*);
wi_result wi_show_session(const wi_session*);

wi_window* wi_make_window(void);
wi_session* wi_make_session(void);

wi_session* wi_add_window_to_session(wi_session*, wi_window*, int);
wi_window* wi_add_content_to_window(wi_window*, const char*, const wi_position);
