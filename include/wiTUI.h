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
} wiBorder;

typedef struct wiSession wiSession;
typedef struct wiWindow wiWindow;
typedef struct wiWindow {
	int width;
	int height;
	// (HEAP)
	char* title;
	// (HEAP)
	char* footer;
	// (HEAP)
	char*** contents;
	wiBorder border;
	bool wrapText;
	bool storeCursorPosition;
	// (HEAP)
	wiWindow** dependingWindows;

	// Only change this outside library code if you like debugging.
    int _internalAmountDepending;
	// Only change this outside library code if you like debugging.
	int _internalAmountRows;
	// Only change this outside library code if you like debugging (HEAP).
	int* _internalAmountCols;
} wiWindow;

typedef struct wiSession {
	// (HEAP)
	wiWindow*** windows;
	bool fullScreen;
	wiPosition cursorStart;
	wiMovementKeys movementKeys;

	// Only change this outside library code if you like debugging.
	int _internalAmountRows;
	// Only change this outside library code if you like debugging (HEAP).
	int* _internalAmountCols;
} wiSession;

typedef struct wiResult {
	wiPosition lastWindow;
	wiPosition lastCursor;
} wiResult;


void wiFreeSessionCompletely(wiSession*);
void wiFreeWindow(wiWindow*);
wiResult wiShowSession(const wiSession*);

wiWindow* wiMakeWindow(void);
wiSession* wiMakeSession(void);

wiSession* wiAddWindowToSession(wiSession*, wiWindow*, int);
wiWindow* wiAddContentToWindow(wiWindow*, const char*, const wiPosition);
