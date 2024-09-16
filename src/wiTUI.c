#include "../include/wiTUI.h"
#include "../../WiTesting/wiAssert.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

wiWindow* wiMakeWindow() {
	wiWindow* window = (wiWindow*) malloc(sizeof(wiWindow));

	window->width = 10;
	window->height = 10;
	window->title = strdup("Test window");
	window->footer = strdup("q: quit");

	// Make 1x1 array initially
	int rows = 1, cols = 1;
	window->contents = (char***) malloc(rows * sizeof(char**));
	for (int i = 0; i < rows; i++) {
		window->contents[i] = (char**) malloc(cols * sizeof(char*));
	}

	wiBorder border = { 
		"\u256D", "\u256E", "\u256F", "\u2570", 
		"\u2500", "\u2500", "\u2502", "\u2502",
		"", "\033[2m"
	};
	window->border = border;

	window->wrapText = true;
	window->storeCursorPosition = true;

	window->dependingWindows = NULL;
	window->parentSession = NULL;

	return window;
}

wiSession* wiMakeSession() {
	wiSession* session = (wiSession*) malloc(sizeof(wiSession));

	// Starting with a 1x1 initially
	int rows = 1, cols = 1;
	session->windows = (wiWindow***) malloc(rows * sizeof(wiWindow**));
	for (int i = 0; i < rows; i++) {
		session->windows[i] = (wiWindow**) malloc(cols * sizeof(wiWindow*));
	}

	session->fullScreen = false;
	session->cursorStart = (wiPosition) { 0, 0 };

	wiMovementKeys mKeys;
	mKeys.left = 'h';
	mKeys.right = 'l';
	mKeys.up = 'k';
	mKeys.down = 'j';
	mKeys.modifierKey = CTRL;
	session->movementKeys = mKeys;

	session->_internalAmountRows = rows;
	session->_internalAmountCols = (int*) malloc (cols * sizeof(int));
	for (int i = 0; i < rows; i++) {
		session->_internalAmountCols[i] = cols;
	}

	return session;
}

// Help function of wiAddWindowToSession()
static inline wiSession* _addWindowToSessionExistingRow(wiSession* session, wiWindow* window, int row) {
	printf("Adding window to existing row\n");
	// Update internal size
	int col = session->_internalAmountCols[row];
	session->_internalAmountCols[row]++;
	printf("Added window to row %i, which now has %i cols\n", row, col+1);

	// Grow row by 1
	session->windows[row] = (wiWindow**) realloc((void*) session->windows[row], (col + 1) * sizeof(wiWindow*));
	wiAssert(session->windows != NULL, "Failed to reallocate memory while adding window to session.");

	// Add window to row
	session->windows[row][col] = window;

	return session;
}

// Help function of wiAddWindowToSession()
static inline wiSession* _addWindowToSessionNewRow(wiSession* session, wiWindow* window, int row) {
	row = session->_internalAmountRows;

	// Update internal size
	session->_internalAmountCols = (int*) realloc(session->_internalAmountCols, session->_internalAmountRows * sizeof(int));
	session->_internalAmountCols[row] = 1;
	session->_internalAmountRows++;

	// Grow row by 1
	session->windows = (wiWindow***) realloc(
			session->windows,
			session->_internalAmountRows * sizeof(wiWindow**)
			);
	wiAssert(session->windows != NULL, "Failed to reallocate memory while adding window to session.");

	// Add window to the end of the row
	session->windows[row] = (wiWindow**) malloc(1 * sizeof(wiWindow*));
	wiAssert(session->windows[row] != NULL, "Failed to allocate memory while adding window to session.");
	session->windows[row][0] = window;

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
wiSession* wiAddWindowToSession(wiSession* session, wiWindow* window, int row) {
	// Add extra row if necessary
	if (row >= session->_internalAmountRows) {
		return _addWindowToSessionNewRow(session, window, row);
	}
	return _addWindowToSessionExistingRow(session, window, row);
}

/*
 * Show the session and return the last cursor position
 */
wiResult wiShowSession(const wiSession* session) {
	wiResult cursor;
	cursor.lastCursor = (wiPosition){ 0, 0 };
	cursor.lastWindow = (wiPosition){ 0, 0 };
	
	return cursor;
}

/*
 * Free the session and all windows inside.
 * 
 * If you want to reuse some windows or even window-contents, 
 * you can not use this function.
 */
void wiFreeSessionCompletely(wiSession* session) {
	// TODO: WIP
}
