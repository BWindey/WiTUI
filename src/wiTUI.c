#include "wi_tui.h"		/* declarations */
#include "wiAssert.h"	/* wiAssert() */

#include <stdatomic.h>  /* atomic_store */
#include <stdbool.h>	/* true, false */
#include <stddef.h>		/* size_t */
#include <stdlib.h>		/* malloc(), realloc(), free() */
#include <string.h>		/* strdup(), strchr(), strlen() */


static wi_content* split_lines(char* content) {
	int internal_amount_lines = 10;
	int amount_lines = 0;
	char** lines = (char**) malloc(internal_amount_lines * sizeof(char*));
	size_t* line_lengths = (size_t*) malloc(internal_amount_lines * sizeof(size_t));

	char* newline = strchr(content, '\n');
	while (newline != NULL) {
		if (amount_lines == internal_amount_lines) {
			internal_amount_lines += 10;
			lines = (char**) realloc(lines, internal_amount_lines * sizeof(char*));
			line_lengths =
				(size_t*) realloc(line_lengths, internal_amount_lines * sizeof(size_t));
		}

		int length = newline - content;
		/* If empty line ('\n\n'), then put a single " " so the cursor stays
		 * visible */
		if (length == 0) {
			lines[amount_lines] = (char*) malloc(2 * sizeof(char));
			strcpy(lines[amount_lines], " ");
			line_lengths[amount_lines] = 1;
		} else {
			lines[amount_lines] = (char*) malloc((length + 1) * sizeof(char));
			memcpy(lines[amount_lines], content, length);
			lines[amount_lines][length] = '\0';
			line_lengths[amount_lines] = length;
		}

		content += length + 1;
		amount_lines++;

		newline = strchr(content, '\n');
	}

	/* Last line in content can be not delimited by '\n' */
	if (newline == NULL && *content != '\0') {
		if (amount_lines == internal_amount_lines) {
			internal_amount_lines += 1;
			lines = (char**) realloc(lines, internal_amount_lines * sizeof(char*));
			line_lengths =
				(size_t*) realloc(lines, internal_amount_lines * sizeof(size_t));
		}

		int length = strlen(content);
		lines[amount_lines] = (char*) malloc((length + 1) * sizeof(char));
		memcpy(lines[amount_lines], content, length);
		lines[amount_lines][length] = '\0';
		line_lengths[amount_lines] = length;

		amount_lines++;
	}

	wi_content* split_contents = (wi_content*) malloc(sizeof(wi_content));
	*split_contents = (wi_content){
		.lines = lines,
		.line_lengths = line_lengths,
		.amount_lines = amount_lines,
		.internal_amount_lines = internal_amount_lines
	};

	return split_contents;
}

wi_window* wi_make_window(void) {
	wi_window* window = (wi_window*) malloc(sizeof(wi_window));

	window->width = 10;
	window->height = 10;

	/* Starting without content */
	window->contents = NULL;
	window->internal.content_rows = 0;
	window->internal.content_cols = NULL;

	window->border = (wi_border) {
		.title = "",
		.footer = "",
		.title_alignment = LEFT,
		.footer_alignment = RIGHT,
		/* Rounded corners */
		"\u256D", "\u256E", "\u256F", "\u2570",
		"\u2502", "\u2500", "\u2502", "\u2500",
		.focussed_colour = "", 			/* Standard (white) */
		.unfocussed_colour = "\033[2m" 	/* Dim */
	};

	window->wrap_text = false;
	window->store_cursor_position = true;
	window->cursor_rendering = POINTBASED;

	window->depends_on = NULL;
	window->internal.depending_windows = NULL;
	window->internal.amount_depending = 0;

	window->internal.content_render_offset = (wi_position) { 0, 0 };
	window->internal.visual_cursor_position = (wi_position) { 0, 0 };
	window->internal.currently_focussed = false;

	return window;
}

wi_session* wi_make_session(void) {
	wi_session* session = (wi_session*) malloc(sizeof(wi_session));

	/* Starting with a 1 empty row */
	int rows = 1;
	session->windows = (wi_window***) malloc(rows * sizeof(wi_window**));
	session->windows[0] = NULL;
	session->internal.amount_rows = rows;
	session->internal.amount_cols = (int*) malloc(rows * sizeof(int));
	session->internal.amount_cols[0] = 0;

	session->start_clear_screen = false;
	session->focus_pos = (wi_position) { 0, 0 };

	/* Start with room for 15, that's enough room for 6 extra keymaps without
	 * re-allocating. Should be enough for most people. */
	int keymap_array_size = 15;
	session->keymaps = (wi_keymap*) malloc(keymap_array_size * sizeof(wi_keymap));
	session->internal.keymap_array_size = keymap_array_size;
	session->internal.amount_keymaps = 0;
	wi_add_keymap_to_session(session, 'h', NONE, wi_scroll_left);
	wi_add_keymap_to_session(session, 'l', NONE, wi_scroll_right);
	wi_add_keymap_to_session(session, 'k', NONE, wi_scroll_up);
	wi_add_keymap_to_session(session, 'j', NONE, wi_scroll_down);
	wi_add_keymap_to_session(session, 'h', CTRL, wi_move_focus_left);
	wi_add_keymap_to_session(session, 'l', CTRL, wi_move_focus_right);
	wi_add_keymap_to_session(session, 'k', CTRL, wi_move_focus_up);
	wi_add_keymap_to_session(session, 'j', CTRL, wi_move_focus_down);
	wi_add_keymap_to_session(session, 'q', NONE, wi_quit_rendering);

	atomic_store(&(session->keep_running), true);

	return session;
}

wi_session* wi_add_keymap_to_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*callback)(const char, wi_session*)
) {
	if (session->internal.amount_keymaps == session->internal.keymap_array_size) {
		session->internal.keymap_array_size += 15;
		session->keymaps = (wi_keymap*) realloc(
			session->keymaps,
			session->internal.keymap_array_size * sizeof(wi_keymap)
		);
	}
	session->keymaps[session->internal.amount_keymaps] = (wi_keymap) {
		.key = key,
		.modifier = modifier,
		.callback = callback
	};
	session->internal.amount_keymaps++;

	return session;
}

wi_session* wi_add_window_to_session(wi_session* session, wi_window* window, int row) {
	/* Add extra row if necessary */
	if (row >= session->internal.amount_rows) {
		row = session->internal.amount_rows;
		session->internal.amount_rows++;

		session->windows = realloc(session->windows, (row + 1) * sizeof(wi_window**));
		wiAssert(session->windows != NULL, "Something went wrong while trying to add a window to a session");
		session->windows[row] = NULL;

		session->internal.amount_cols = realloc(session->internal.amount_cols, (row + 1) * sizeof(int));
		wiAssert(session->internal.amount_cols != NULL, "Something went wrong while tring to add a window to a session");
		session->internal.amount_cols[row] = 0;
	}

	/* Grow the row */
	int col = session->internal.amount_cols[row];
	session->internal.amount_cols[row]++;
	session->windows[row] = realloc(session->windows[row], session->internal.amount_cols[row] * sizeof(wi_window*));
	wiAssert(session->windows[row] != NULL, "Something went wrong while tring to add a window to a session");

	session->windows[row][col] = window;

	return session;
}

wi_window* wi_add_content_to_window(
	wi_window* window,
	char* content,
	const wi_position position
) {
	wi_content* split_content = split_lines(content);

	/* Grow rows if necessary */
	if (position.row >= window->internal.content_rows) {
		window->contents = (wi_content***) realloc(
			window->contents,
	 		(position.row + 1) * sizeof(wi_content**)
		);
		wiAssert(window->contents != NULL, "Failed to grow array when adding content to a window");

		window->internal.content_cols = (int*) realloc(
			window->internal.content_cols,
	 		(position.row + 1) * sizeof(int)
		);
		wiAssert(window->internal.content_cols != NULL, "Failed to grow array when adding content to a window");

		/* Fill in the spaces between old and new */
		for (int i = window->internal.content_rows; i <= position.row; i++) {
			window->contents[i] = NULL;
			window->internal.content_cols[i] = 0;
		}
		window->internal.content_rows = position.row + 1;
	}

	/* Grow cols if necessary */
	if (position.col >= window->internal.content_cols[position.row]) {
		window->contents[position.row] = (wi_content**) realloc(
			window->contents[position.row],
			(position.col + 1) * sizeof(wi_content*)
		);
		wiAssert(window->contents[position.row] != NULL, "Failed to grow array when adding content to a window");

		/* Fill in the spaces between old and new */
		for (int i = window->internal.content_cols[position.row]; i < position.col; i++) {
			window->internal.content_cols[i] = 0;
		}

		window->internal.content_cols[position.row] = position.col + 1;
	}

	window->contents[position.row][position.col] = split_content;

	return window;
}

void wi_free_session(wi_session* session) {
	/* Free all the windows... Yay */
	for (int i = 0; i < session->internal.amount_rows; i++) {
		for (int j = 0; j < session->internal.amount_cols[i]; j++) {
			wi_free_window(session->windows[i][j]);
		}
		free(session->windows[i]);
	}
	free(session->windows);
	free(session->internal.amount_cols);
	free(session->keymaps);
	free(session);
}

void wi_free_window(wi_window* window) {
	free(window->internal.depending_windows);

	for (int i = 0; i < window->internal.content_rows; i++) {
		for (int j = 0; j < window->internal.content_cols[i]; j++) {
			wi_free_content(window->contents[i][j]);
		}
		free(window->contents[i]);
	}
	free(window->contents);
	free(window->internal.content_cols);
	free(window);
}

void wi_free_content(wi_content* content) {
	for (int i = 0; i < content->amount_lines; i++) {
		free(content->lines[i]);
	}
	free(content->lines);
	free(content->line_lengths);
	free(content);
}
