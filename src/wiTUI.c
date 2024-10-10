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

	// Starting with a 1 empty row
	int rows = 1;
	window->contents = (char***) malloc(rows * sizeof(char**));
	window->contents[0] = NULL;
	window->_internalAmountRows = rows;
	window->_internalAmountCols = (int*) malloc(rows * sizeof(int*));

	// Rounded corners, standard focus colour and dim unfocussed colour
	window->border = (wiBorder) { 
		"\u256D", "\u256E", "\u256F", "\u2570", 
		"\u2502", "\u2502", "\u2500", "\u2500",
		"", "\033[2m"
	};

	window->wrapText = true;
	window->storeCursorPosition = true;

	window->dependingWindows = NULL;
	window->_internalAmountDepending = 0;

	return window;
}

wiSession* wiMakeSession() {
	wiSession* session = (wiSession*) malloc(sizeof(wiSession));

	// Starting with a 1 empty row
	int rows = 1;
	session->windows = (wiWindow***) malloc(rows * sizeof(wiWindow**));
	session->windows[0] = NULL;
	session->_internalAmountRows = rows;
	session->_internalAmountCols = (int*) malloc(rows * sizeof(int*));

	session->fullScreen = false;
	session->cursorStart = (wiPosition) { 0, 0 };

	wiMovementKeys mKeys;
	mKeys.left = 'h';
	mKeys.right = 'l';
	mKeys.up = 'k';
	mKeys.down = 'j';
	mKeys.modifierKey = CTRL;
	session->movementKeys = mKeys;

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
		row = session->_internalAmountRows;
		session->_internalAmountRows++;

		session->windows = realloc(session->windows, (row + 1) * sizeof(wiWindow**));
		wiAssert(session->windows != NULL, "Something went wrong while trying to add a window to a session");
		session->windows[row] = NULL;

		session->_internalAmountCols = realloc(session->_internalAmountCols, (row + 1) * sizeof(int));
		wiAssert(session->_internalAmountCols != NULL, "Something went wrong while tring to add a window to a session");
		session->_internalAmountCols[row] = 0;
	}

	// Grow the row 
	int col = session->_internalAmountCols[row];
	session->_internalAmountCols[row]++;
	session->windows[row] = realloc(session->windows[row], session->_internalAmountCols[row]);

	session->windows[row][col] = window;

	return session;
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
