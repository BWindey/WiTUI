#include "../include/wiTUI.h"			/* declarations */
#include "../../WiTesting/wiAssert.h"	/* wiAssert() */

#include <stdbool.h>	/* true, false */
#include <stdlib.h>		/* malloc(), realloc(), free() */
#include <string.h>		/* strdup() */

wi_window* wi_make_window(void) {
	wi_window* window = (wi_window*) malloc(sizeof(wi_window));

	window->width = 10;
	window->_internal_rendered_width = 10;
	window->_internal_rendered_height = 10;
	window->height = 10;
	window->title = "Test window";
	window->footer = "q: quit";
	window->title_alignment = LEFT;
	window->footer_alignment = RIGHT;

	/* Starting with a 1 empty row*/
	int rows = 1;
	window->contents = (char***) malloc(rows * sizeof(char**));
	window->contents[0] = NULL;
	window->_internal_content_rows = rows;
	window->_internal_content_cols = (int*) malloc(rows * sizeof(int));
	window->_internal_content_cols[0] = 0;

	/* Rounded corners, standard focus colour and dim unfocussed colour */
	window->border = (wi_border) {
		"\u256D", "\u256E", "\u256F", "\u2570",
		"\u2502", "\u2502", "\u2500", "\u2500",
		"", "\033[2m"
	};

	window->wrapText = true;
	window->store_cursor_position = true;
	window->cursor_rendering = POINTBASED;

	window->depending_windows = NULL;
	window->_internal_amount_depending = 0;

	window->_internal_last_cursor_position = (wi_position) { 0, 0 };
	window->_internal_currently_focussed = false;

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
	mKeys.quit = 'q';
	mKeys.modifier_key = CTRL;
	session->movement_keys = mKeys;

	return session;
}

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

wi_window* wi_add_content_to_window(
	wi_window* window,
	char* content,
	const wi_position position
) {
	/* Grow rows if necessary */
	if (position.row >= window->_internal_content_rows) {
		window->contents = (char***) realloc(
			window->contents,
	 		(position.row + 1) * sizeof(char**)
		);
		wiAssert(window->contents != NULL, "Failed to grow array when adding content to a window");

		window->_internal_content_cols = (int*) realloc(
			window->_internal_content_cols,
	 		(position.row + 1) * sizeof(int)
		);
		wiAssert(window->_internal_content_cols != NULL, "Failed to grow array when adding content to a window");

		/* Fill in the spaces between old and new */
		for (int i = window->_internal_content_rows; i < position.row; i++) {
			window->contents[i] = NULL;
			window->_internal_content_cols[i] = 0;
		}
		window->_internal_content_rows = position.row + 1;
	}

	/* Grow cols if necessary */
	if (position.col >= window->_internal_content_cols[position.row]) {
		window->contents[position.row] = (char**) realloc(
			window->contents[position.row],
			(position.col + 1) * sizeof(char*)
		);
		wiAssert(window->contents[position.row] != NULL, "Failed to grow array when adding content to a window");

		/* Fill in the spaces between old and new */
		for (int i = window->_internal_content_cols[position.row]; i < position.col; i++) {
			window->_internal_content_cols[i] = 0;
		}

		window->_internal_content_cols[position.row] = position.col + 1;
	}

	window->contents[position.row][position.col] = content;

	return window;
}

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

void wi_free_window(wi_window* window) {
	free(window->depending_windows);

	for (int i = 0; i < window->_internal_content_rows; i++) {
		free(window->contents[i]);
	}
	free(window->contents);
	free(window->_internal_content_cols);
	free(window);
}
