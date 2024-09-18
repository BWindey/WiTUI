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
typedef struct wiWindow wiWindow;
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
	wiWindow** dependingWindows;

	// Only change this outside library code if you like debugging.
    int _internalAmountDepending;
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


void wiFreeSessionCompletely(wiSession* session);
wiResult wiShowSession(const wiSession* session);

wiWindow* wiMakeWindow();
wiSession* wiMakeSession();

wiSession* wiAddWindowToSession(wiSession*, wiWindow*, int);
wiWindow* wiAddContentToWindow(wiWindow*, const char*, const wiPosition);


