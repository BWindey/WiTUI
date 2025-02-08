#ifndef WI_TUI_DATA_HEADER_GUARD
#define WI_TUI_DATA_HEADER_GUARD

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
 * To support UTF8 and anssi escape sequences, I need this to tell me how long
 * a "character" in a string is. This struct is the result of
 * `wi_char_byte_size()`, it contains `.bytes` and `.width` (visual).
 */
typedef struct wi_string_length wi_string_length;

/*
 * A wrapper around a `wi_string` and a list of `wi_string_view`s that will
 * be the lines to display.
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
 * A container for wi_window's. Holds a 2D array of windows, whether to clear
 * the screen before rendering, which window is focussed, a list of wi_keymap's,
 * 2 atomic-bools keep_running and need_rerender.
 * Holds an aditional internal struct for bookkeeping.
 */
typedef struct wi_session wi_session;

/*
 * A wrapper around `char*` to include string-length.
 * Does not use a delimiting '\0'.
 * Owns the `char*` memory.
 * The length is given in `wi_code_lengths` to include both byte length and
 * visual length, because that's interesting for this library.
 */
typedef struct wi_string wi_string;

/*
 * Same as a `wi_string`, but does not own the `char*` memory.
 */
typedef struct wi_string wi_string_view;

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

struct wi_string_length {
	unsigned int width;
	unsigned int bytes;
};

struct wi_string {
	char* string;
	wi_string_length length;
};

struct wi_content {
	wi_string_view original;
	wi_string_view* line_list;
	int amount_lines;
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

struct wi_session {
	/* (HEAP) */
	wi_window*** windows;
	bool start_clear_screen;
	wi_position focus_pos;
	wi_keymap* keymaps;

	bool keep_running;
	atomic_bool need_rerender;
	bool running_render_thread;

	/* Internal variables, do not change outside library-code,
	 * unless you really know what you're doing. */
	struct {
		int amount_rows;
		int* amount_cols;
		int capacity_rows;
		int* capacity_cols;

		int keymap_array_size;
	} internal;
};

struct wi_window {
	int width;
	int height;

	/* (HEAP) */
	wi_content** content_grid;
	wi_border border;

	bool wrap_text;

	wi_cursor_rendering cursor_rendering;

	wi_window* depends_on;

	/* Internal variables, do not change outside library-code,
	 * unless you really know what you're doing. */
	struct {
		int rendered_width;
		int rendered_height;

		int content_grid_row_capacity;
		int* content_grid_col_capacity;

		/* (HEAP) */
		wi_window** depending_windows;
		int amount_depending;
		int depending_capacity;

		/* Cursor */
		wi_position visual_cursor;	/* In visual chars */
		wi_position offset_cursor;	/* In visual chars */

		bool currently_focussed;
	} internal;
};

#endif /* !WI_TUI_DATA_HEADER_GUARD */
