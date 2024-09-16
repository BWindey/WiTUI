#pragma once

#include <stdbool.h>

typedef struct wiPosition {
	int row;
	int col;
} wiPosition;

typedef enum wiModifier {
	CTRL, ALT, SHIFT, SUPER
} wiModifier;

typedef struct wiMovementKeys {
	char left;
	char right;
	char up;
	char down;
	wiModifier modifierKey;
} wiMovementKeys;

typedef struct wiBorder {
	char* corner1;
	char* corner2;
	char* corner3;
	char* corner4;
	char* left;
	char* right;
	char* top;
	char* bottom;
	char* foregroundColour;
	char* backgroundColour;
} wiBorder;

typedef struct wiSession wiSession;
typedef struct wiWindow {
	int width;
	int height;
	// Al string related things will be copies
	char* title;
	char* footer;
	char*** contents;
	wiBorder border;
	bool wrapText;
	bool storeCursorPosition;
	wiPosition* dependingWindows;
	wiSession* parentSession;
} wiWindow;

typedef struct wiSession {
	wiWindow*** windows;
	bool fullScreen;
	wiPosition cursorStart;
	wiMovementKeys movementKeys;

	// Only change this outside library code if you like debugging.
	int _internalAmountRows;
	// Only change this outside library code if you like debugging.
	int* _internalAmountCols;
} wiSession;

typedef struct wiResult {
	wiPosition lastWindow;
	wiPosition lastCursor;
} wiResult;


// Free the session, all the windows inside, and their contents
void wiFreeSessionCompletely(wiSession* session);
// Render the session, and return the result once exited
wiResult wiShowSession(const wiSession* session);

// Utility functions to make default windows and sessions
wiWindow* wiMakeWindow();
wiSession* wiMakeSession();

// Utility function to do the work of adding a window to a session
// Will place the window at the given row, unless row-1 is bigger then the
// current amount of rows, it will place it on a new row under the last
wiSession* wiAddWindowToSession(wiSession*, wiWindow*, int);

// Utility function to add content to a specific location. This will oxpand the
// contents-array of the window if needed, and overwrite existing content if 
// there is any at the location
wiWindow* wiAddContentToWindow(wiWindow*, const char*, const wiPosition);
