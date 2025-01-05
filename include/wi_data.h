#ifndef WI_TUI_DATA_HEADER_GUARD
#define WI_TUI_DATA_HEADER_GUARD

#include <stddef.h>		/* size_t */
#include <stdbool.h>	/* bool */
#include <stdatomic.h>	/* atomic_bool */

/*
 * Some of the comments are bad comments. They tell what you clearly see,
 * instead of explaining why you see what you see.
 * I justify this however by telling you that my LSP does not show me the
 * content of a struct when hovering over it, but it does show the comment
 * that I wrote just above the struct.
 *
 * There is however an open pull-request for llvm (#89557) that would then
 * trickle down into clangd, so maybe this won't be necessary in the future.
 */



/* ----------------------------------------------------------
 * Forward declarations, typedefs and LSP-hover documentation
 * ---------------------------------------------------------- */

/*
 * A struct to hold border-info: title, footer, title-alignment,
 * footer-alignment, corner (-top-left and anti-clockwise continue),
 * side (-left and anti-clockwise continue),
 * focussed- and unfocussed-colour.
 * If the border should not be displayed, set the elements to 0/NULL,
 * specifically .bottom_left. Empty border-pieces should be empty strings.
 */
typedef struct wi_border wi_border;

/*
 * Represents an array of lines that form the content of a window.
 * Contains the lines, line_lengths, actual amount_lines, and
 * internal_amount_lines for memory-management.
 */
typedef struct wi_content wi_content;

/*
 * One keymap, containing a key in 'a-z' range, a wi_modifier, and a
 * callback function that gets executed when the key is pressed.
 */
typedef struct wi_keymap wi_keymap;

/* A simple struct with .row and .col */
typedef struct wi_position wi_position;

/*
 * A package for 2 wi_position's.
 * Used to return the last focussed window in a session, and the cursor-position
 * in that last focussed window.
 */
typedef struct wi_result wi_result;

/*
 * A container for wi_window's. Holds a 2D array of windows, whether to clear
 * the screen before rendering, which window is focussed, a list of wi_keymap's,
 * 2 atomic-bools keep_running and need_rerender.
 * Holds an aditional internal struct for bookkeeping.
 */
typedef struct wi_session wi_session;

/*
 * A container for wi_content's. Holds a 2D array of contents, a width and
 * height, a wi_border, whether to wrap text and/or store the cursor-position,
 * how to render the cursor, which are the depending windows, and which it
 * depends on.
 * Holds an aditional internal struct for bookkeeping.
 */
typedef struct wi_window wi_window;



/* ------------------------------------------------------------------
 * Enums, they - for some reason - can not have a forward declaration
 * ------------------------------------------------------------------ */

/* Key-modifier, supporting: CTRL, ALT, SHIFT, NONE */
typedef enum wi_modifier {
	CTRL, ALT, SHIFT, NONE
} wi_modifier;

/* Possible cursor-renderings: INVISIBLE, LINEBASED, POINTBASED */
typedef enum wi_cursor_rendering {
	INVISIBLE, LINEBASED, POINTBASED
} wi_cursor_rendering;

/* Info (title or footer) -alignment: LEFT, CENTER, RIGHT */
typedef enum wi_info_alignment {
	LEFT, CENTER, RIGHT
} wi_info_alignment;



/* ------------------------------
 * Data-structures used by WiTUI.
 * ------------------------------ */

struct wi_border {
	char* title;
	char* footer;
	wi_info_alignment title_alignment;
	wi_info_alignment footer_alignment;

	char* corner_top_left;
	char* corner_top_right;
	char* corner_bottom_right;
	char* corner_bottom_left;
	char* side_left;
	char* side_top;
	char* side_right;
	char* side_bottom;

	char* focussed_colour;
	char* unfocussed_colour;
};

struct wi_content {
	/* (HEAP) */
	char** lines;
	/* (HEAP) Lengths excluding '\0', in bytes. */
	unsigned int* line_lengths_chars;
	/* (HEAP) Lengths excluding '\0', in bytes. */
	unsigned int* line_lengths_bytes;

	int amount_lines;
	int internal_amount_lines;
};

struct wi_keymap {
	wi_modifier modifier;
	char key;
	void (*callback)(const char, wi_session*);
};

struct wi_position {
	int row;
	int col;
};

struct wi_result {
	wi_position last_window;
	wi_position last_cursor;
};

struct wi_session {
	/* (HEAP) */
	wi_window*** windows;
	bool start_clear_screen;
	wi_position focus_pos;
	wi_keymap* keymaps;

	atomic_bool keep_running;
	atomic_bool need_rerender;

	/* Internal variables, do not change outside library-code,
	 * unless you really know what you're doing. */
	struct {
		int amount_rows;
		int* amount_cols;
		int amount_keymaps;
		int keymap_array_size;
	} internal;
};

struct wi_window {
	int width;
	int height;

	/* (HEAP) */
	wi_content*** contents;
	wi_border border;

	bool wrap_text;
	bool store_cursor_position;

	wi_cursor_rendering cursor_rendering;

	wi_window* depends_on;

	/* Internal variables, do not change outside library-code,
	 * unless you really know what you're doing. */
	struct {
		int amount_depending;

		int content_rows;
		/* (HEAP) */
		int* content_cols;

		int rendered_width;
		int rendered_height;

		/* (HEAP) */
		wi_window** depending_windows;

		wi_position content_offset_chars;
		wi_position content_offset_bytes;
		wi_position visual_cursor_position;

		bool currently_focussed;
	} internal;
};

#endif /* !WI_TUI_DATA_HEADER_GUARD */
