#include "../include/wiTUI.h"
#include "../../WiTesting/wiAssert.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

wi_window* wi_make_window(void) {
	wi_window* window = (wi_window*) malloc(sizeof(wi_window));

	window->width = 10;
	window->height = 10;
	window->title = strdup("Test window");
	window->footer = strdup("q: quit");

	/* Starting with a 1 empty row*/
	int rows = 1;
	window->contents = (char***) malloc(rows * sizeof(char**));
	window->contents[0] = NULL;
	window->_internal_amount_rows = rows;
	window->_internal_amount_cols = (int*) malloc(rows * sizeof(int));
	window->_internal_amount_cols[0] = 0;

	/* Rounded corners, standard focus colour and dim unfocussed colour */
	window->border = (wi_border) { 
		"\u256D", "\u256E", "\u256F", "\u2570", 
		"\u2502", "\u2502", "\u2500", "\u2500",
		"", "\033[2m"
	};

	window->wrapText = true;
	window->store_cursor_position = true;

	window->depending_windows = NULL;
	window->_internal_amount_depending = 0;

	return window;
}

wi_session* wi_make_session() {
	wi_session* session = (wi_session*) malloc(sizeof(wi_session));

	/* Starting with a 1 empty row */
	int rows = 1;
	session->windows = (wi_window***) malloc(rows * sizeof(wi_window**));
	session->windows[0] = NULL;
	session->_internal_amount_rows = rows;
	session->_internal_amount_cols = (int*) malloc(rows * sizeof(int));
	session->_internal_amount_cols[0] = 0;

	session->full_screen = false;
	session->cursor_start = (wi_position) { 0, 0 };

	wi_movement_keys mKeys;
	mKeys.left = 'h';
	mKeys.right = 'l';
	mKeys.up = 'k';
	mKeys.down = 'j';
	mKeys.modifier_key = CTRL;
	session->movement_keys = mKeys;

	return session;
}

/*
 * Add a window to a session on a specified row.
 * This will do all the work so the caller doesn't have to.
 * When the row is bigger then the current amount of rows +1,
 * the window will be placed on a new row below the current last row.
 *
 * Returns the updated session.
 */
wi_session* wi_add_window_to_session(wi_session* session, wi_window* window, int row) {
	/* Add extra row if necessary */
	if (row >= session->_internal_amount_rows) {
		row = session->_internal_amount_rows;
		session->_internal_amount_rows++;

		session->windows = realloc(session->windows, (row + 1) * sizeof(wi_window**));
		wiAssert(session->windows != NULL, "Something went wrong while trying to add a window to a session");
		session->windows[row] = NULL;

		session->_internal_amount_cols = realloc(session->_internal_amount_cols, (row + 1) * sizeof(int));
		wiAssert(session->_internal_amount_cols != NULL, "Something went wrong while tring to add a window to a session");
		session->_internal_amount_cols[row] = 0;
	}

	/* Grow the row */
	int col = session->_internal_amount_cols[row];
	session->_internal_amount_cols[row]++;
	session->windows[row] = realloc(session->windows[row], session->_internal_amount_cols[row] * sizeof(wi_window*));
	wiAssert(session->windows[row] != NULL, "Something went wrong while tring to add a window to a session");

	session->windows[row][col] = window;

	return session;
}

/*
 * Show the session and return the last cursor position
 */
wi_result wi_show_session(const wi_session* session) {
	/* TODO: */
	wi_result cursor;
	cursor.last_cursor = (wi_position){ 0, 0 };
	cursor.last_window = (wi_position){ 0, 0 };

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		wi_window* window = session->windows[row][0];
		wi_border border = window->border;

		/* Top border */
		printf("%s", border.focussed_colour);
		printf("%s", border.corner_top_left);

		for (int i = 0; i < window->width; i++) {
			printf("%s", border.side_top);
		}

		printf("%s\n", border.corner_top_right);

		/* Contents (empty for now) */
		for (int i = 0; i < window->height; i++) {
			printf("%s", border.side_left);
			for (int j = 0; j < window->width; j++) {
				printf("\033[0m%c%s", 'c', border.focussed_colour); 		/* This will need to print content */
			}
			printf("%s\n", border.side_right);
		}

		/* Top border */
		printf("%s", border.corner_bottom_left);

		for (int i = 0; i < window->width; i++) {
			printf("%s", border.side_bottom);
		}

		printf("%s\033[0m\n", border.corner_bottom_right);
	}

	return cursor;
}

/*
 * Free the session and all windows inside.
 * 
 * If you want to reuse some windows or even window-contents, 
 * you can not use this function.
 */
void wi_free_session_completely(wi_session* session) {
	/* Free all the windows... Yay */
	for (int i = 0; i < session->_internal_amount_rows; i++) {
		for (int j = 0; j < session->_internal_amount_cols[i]; j++) {
			wi_free_window(session->windows[i][j]);
		}
		free(session->windows[i]);
	}
	free(session->windows);
	free(session->_internal_amount_cols);
	free(session);
}

/* 
 * Free the window and all contents inside.
 *
 * Does NOT free depending windows, only the array of pointers to them.
 *
 * If you want to reuse some contents or borders,
 * you'll have to do some manual work.
 */
void wi_free_window(wi_window* window) {
	free(window->title);
	free(window->footer);

	free(window->depending_windows);

	for (int i = 0; i < window->_internal_amount_rows; i++) {
		for (int j = 0; j < window->_internal_amount_cols[i]; j++) {
			free(window->contents[i][j]);
		}
		free(window->contents[i]);
	}
	free(window->contents);
	free(window->_internal_amount_cols);
	free(window);
}
