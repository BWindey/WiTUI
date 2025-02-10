#include "wiAssert.h"	/* wiAssert() */
#include "wi_data.h"
#include "wi_internals.h"
#include "wi_functions.h"

#include <stdbool.h>	/* true, false */
#include <stddef.h>		/* size_t */
#include <stdlib.h>		/* malloc(), realloc(), free() */
#include <string.h>		/* strdup(), strchr(), strlen() */
#include <threads.h>	/* thrd_sleep() */

/* Fore safety this is undeffed at the end of the file */
#define MALLOC_ARRAY(ARRAY, SIZE, TYPE) \
	ARRAY = (TYPE*) malloc((SIZE) * sizeof(TYPE)); \
	wiAssert(ARRAY != NULL, "Failed to allocate array '" #ARRAY "'");

#define CALLOC_ARRAY(ARRAY, SIZE, TYPE, INIT) \
	ARRAY = (TYPE*) malloc((SIZE) * sizeof(TYPE)); \
	wiAssert(ARRAY != NULL, "Failed to allocate array '" #ARRAY "'"); \
	for (int __i = 0; __i < (SIZE); __i++) { \
		ARRAY[__i] = (TYPE) INIT; \
	}

#define REALLOC_ARRAY(ARRAY, NEW_SIZE, TYPE) \
	ARRAY = (TYPE*) realloc(ARRAY, (NEW_SIZE) * sizeof(TYPE)); \
	wiAssert(ARRAY != NULL, "Failed to grow array '" #ARRAY "'");

#define RECALLOC_ARRAY(ARRAY, NEW_SIZE, TYPE, OLD_SIZE, INIT) \
	ARRAY = (TYPE*) realloc(ARRAY, (NEW_SIZE) * sizeof(TYPE)); \
	wiAssert(ARRAY != NULL, "Failed to grow array '" #ARRAY "'"); \
	for (int __i = OLD_SIZE; __i < NEW_SIZE; __i++) { \
		ARRAY[__i] = (TYPE) INIT; \
	}

wi_window* wi_make_window(void) {
	wi_window* window = (wi_window*) malloc(sizeof(wi_window));

	window->width = 10;
	window->height = 10;
	window->internal.rendered_width = 10;
	window->internal.rendered_height = 10;

	/* Start with 2x2 array */
	MALLOC_ARRAY(window->content_grid, 2, wi_content*);
	CALLOC_ARRAY(window->content_grid[0], 2, wi_content, { .original.string = NULL });
	CALLOC_ARRAY(window->content_grid[1], 2, wi_content, { .original.string = NULL });

	window->internal.content_grid_row_capacity = 2;
	CALLOC_ARRAY(window->internal.content_grid_col_capacity, 2, int, 2);

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
	window->cursor_rendering = POINTBASED;

	window->depends_on = NULL;
	CALLOC_ARRAY(window->internal.depending_windows, 2, wi_window*, NULL);
	window->internal.amount_depending = 0;
	window->internal.depending_capacity = 2;

	window->internal.offset_cursor = (wi_position) { 0, 0 };
	window->internal.visual_cursor = (wi_position) { 0, 0 };
	window->internal.currently_focussed = false;

	return window;
}

wi_session* wi_make_session(bool add_vim_keybindings) {
	wi_session* session = (wi_session*) malloc(sizeof(wi_session));

	/* Starting with a 2x2 empty grid */
	MALLOC_ARRAY(session->windows, 2, wi_window**);
	MALLOC_ARRAY(session->windows[0], 2, wi_window*);
	MALLOC_ARRAY(session->windows[1], 2, wi_window*);

	session->internal.amount_rows = 0;
	session->internal.capacity_rows = 2;
	CALLOC_ARRAY(session->internal.amount_cols, 2, int, 0);
	CALLOC_ARRAY(session->internal.capacity_cols, 2, int, 2);

	session->start_clear_screen = false;
	session->focus_pos = (wi_position) { 0, 0 };

	/* Start with room for 15, so that when used with the standard vim keybinds,
	 * there is still room for 6 custom keymaps to minimise reallocs. */
	int keymap_array_size = 15;
	CALLOC_ARRAY(
		session->keymaps, keymap_array_size, wi_keymap, { .callback = NULL }
	);
	session->internal.keymap_array_size = keymap_array_size;

	if (add_vim_keybindings) {
		wi_add_keymap_to_session(session, 'h', NONE, wi_scroll_left);
		wi_add_keymap_to_session(session, 'j', NONE, wi_scroll_down);
		wi_add_keymap_to_session(session, 'k', NONE, wi_scroll_up);
		wi_add_keymap_to_session(session, 'l', NONE, wi_scroll_right);
		wi_add_keymap_to_session(session, 'h', CTRL, wi_move_focus_left);
		wi_add_keymap_to_session(session, 'j', CTRL, wi_move_focus_down);
		wi_add_keymap_to_session(session, 'k', CTRL, wi_move_focus_up);
		wi_add_keymap_to_session(session, 'l', CTRL, wi_move_focus_right);
		wi_add_keymap_to_session(session, 'q', NONE, wi_quit_rendering);
	}

	session->keep_running = true;
	session->running_render_thread = false;

	return session;
}

wi_session* wi_add_keymap_to_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*callback)(const char, wi_session*)
) {
	/* First check if there is an available spot inside the array */
	for (int i = 0; i < session->internal.keymap_array_size; i++) {
		if (session->keymaps[i].callback == NULL) {
			session->keymaps[i] = (wi_keymap) {
				.modifier = modifier,
				.key = key,
				.callback = callback
			};
			return session;
		}
	}

	/* If no spot, increase array size. */
	RECALLOC_ARRAY(
		session->keymaps,
		session->internal.keymap_array_size + 15,
		wi_keymap,
		session->internal.keymap_array_size,
		{ .callback = NULL }
	)
	session->keymaps[session->internal.keymap_array_size] = (wi_keymap) {
		.key = key,
		.modifier = modifier,
		.callback = callback
	};
	session->internal.keymap_array_size += 15;

	return session;
}

wi_session* wi_pop_keymap_from_session(
	wi_session* session, const char key, const wi_modifier modifier
) {
	wi_keymap map;
	for (int i = 0; i < session->internal.keymap_array_size; i++) {
		map = session->keymaps[i];
		if (map.callback != NULL && map.key == key && map.modifier == modifier){
			session->keymaps[i].callback = NULL;
			return session;
		}
	}
	return session;
}

wi_session* wi_update_keymap_from_session(
	wi_session* session, const char key, const wi_modifier modifier,
	void (*new_callback)(const char, wi_session*)
) {
	wi_keymap map;
	for (int i = 0; i < session->internal.keymap_array_size; i++) {
		map = session->keymaps[i];
		if (map.callback != NULL && map.key == key && map.modifier == modifier){
			session->keymaps[i].callback = new_callback;
			return session;
		}
	}
	return session;
}

wi_session* wi_add_window_to_session(wi_session* session, wi_window* window, int row) {
	/* When row too big, just add to new row at end. */
	if (row >= session->internal.amount_rows) {
		row = session->internal.amount_rows;
		session->internal.amount_rows++;
	}
	/* Grow array of window-rows if needed. */
	if (row >= session->internal.capacity_rows) {
		int old_capacity = session->internal.capacity_rows;
		int new_capacity = session->internal.capacity_rows * 2;
		session->internal.capacity_rows = new_capacity;
		REALLOC_ARRAY(session->windows, new_capacity, wi_window**);

		/* Initialise new rows */
		for (int i = old_capacity; i < new_capacity; i++) {
			MALLOC_ARRAY(session->windows[i], 2, wi_window*);
		}

		/* Also grow amount_cols */
		RECALLOC_ARRAY(
			session->internal.amount_cols, new_capacity, int, old_capacity, 0
		);
		RECALLOC_ARRAY(
			session->internal.capacity_cols, new_capacity, int, old_capacity, 2
		);
	}

	/* Ensure that there is enough room on the row */
	int amount_on_row = session->internal.amount_cols[row];
	int capacity_on_row = session->internal.capacity_cols[row];
	if (amount_on_row + 1 >= capacity_on_row) {
		REALLOC_ARRAY(session->windows[row], capacity_on_row * 2, wi_window*);
		session->internal.capacity_cols[row] *= 2;
	}

	session->windows[row][amount_on_row] = window;
	session->internal.amount_cols[row] += 1;

	return session;
}

wi_window* wi_add_content_to_window(wi_window* window, char* content, const wi_position position) {
	wi_content processed_content;
	if (window->wrap_text) {
		processed_content = (wi_content) {
			.original.string = content,
			.line_list = NULL
		};
	} else {
		processed_content = split_lines(content);
	}

	/* Make new rows if needed */
	int old_row_capacity = window->internal.content_grid_row_capacity;
	if (position.row >= old_row_capacity) {
		int new_capacity = position.row + 1 > old_row_capacity * 2
			? position.row + 1 : old_row_capacity * 2;
		REALLOC_ARRAY(window->content_grid, new_capacity, wi_content*);

		/* Fill in the spaces between old and new */
		for (int i = old_row_capacity; i < new_capacity; i++) {
			CALLOC_ARRAY(
				window->content_grid[i], 2,
				wi_content, { .original.string = NULL }
			);
		}
		/* And grow the col_capacity array with the new place we got */
		RECALLOC_ARRAY(
			window->internal.content_grid_col_capacity, new_capacity, int,
			old_row_capacity, 2
		);
		window->internal.content_grid_row_capacity = new_capacity;
	}

	/* Grow row if needed */
	int old_col_capacity =
		window->internal.content_grid_col_capacity[position.row];
	if (position.col >= old_col_capacity) {
		int new_capacity = position.col + 1 > old_col_capacity * 2
			? position.col + 1 : old_col_capacity * 2;
		RECALLOC_ARRAY(
			window->content_grid[position.row], new_capacity, wi_content,
			old_col_capacity, { .original.string = NULL }
		);

		window->internal.content_grid_col_capacity[position.row] = position.col + 1;
	}

	window->content_grid[position.row][position.col] = processed_content;

	return window;
}

void wi_bind_dependency(wi_window* parent, wi_window* depending) {
	depending->depends_on = parent;

	/* Grow array by one and add */
	parent->internal.amount_depending++;
	parent->internal.depending_windows = realloc(
		parent->internal.depending_windows,
		parent->internal.amount_depending * sizeof(wi_window*)
	);
	wiAssert(
		parent->internal.depending_windows != NULL,
		"Failed to grow array when adding depending window"
	);
	parent->internal.depending_windows[parent->internal.amount_depending - 1] =
		depending;
}

wi_window* wi_get_focussed_window(wi_session* session) {
	int s_cursor_row = session->focus_pos.row;
	int s_cursor_col = session->focus_pos.col;
	if (s_cursor_col >= session->internal.amount_cols[s_cursor_row]) {
		s_cursor_col = session->internal.amount_cols[s_cursor_row] - 1;
	}

	return session->windows[s_cursor_row][s_cursor_col];
}

wi_content wi_get_current_window_content(const wi_window* window) {
	wiAssert(
		window->content_grid != NULL,
		"Window does not containt any contents!"
	);
	if (window->depends_on == NULL) {
		return window->content_grid[0][0];
	}

	wi_window* dep = window->depends_on;
	wi_position dep_visual_cursor_pos = dep->internal.visual_cursor;
	wi_position dep_content_offset = dep->internal.offset_cursor;

	int row = dep_visual_cursor_pos.row + dep_content_offset.row;
	int col = dep_visual_cursor_pos.col + dep_content_offset.col;

	if (row >= window->internal.content_grid_row_capacity) {
		row = window->internal.content_grid_row_capacity - 1;
	}
	if (col >= window->internal.content_grid_col_capacity[row]) {
		col = window->internal.content_grid_col_capacity[row] - 1;
	}

	while (col > 0 && window->content_grid[row][col].original.string == NULL) {
		col--;
	}
	while (row > 0 && window->content_grid[row][col].original.string == NULL) {
		row--;
	}

	wiAssert(
		window->content_grid[row][col].original.string != NULL,
		"Could not find non-NULL content for a depending window"
	);

	return window->content_grid[row][col];
}

wi_position wi_get_window_cursor_pos(const wi_window *window) {
	const wi_content window_content = wi_get_current_window_content(window);
	const wi_position visual = window->internal.visual_cursor;
	const wi_position offset = window->internal.offset_cursor;

	wi_position actual = (wi_position) {
		.row = visual.row + offset.row,
		.col = visual.col + offset.col
	};

	if (actual.row >= window_content.amount_lines) {
		actual.row = window_content.amount_lines - 1;
	}
	if (actual.col >= (int) window_content.line_list[actual.row].length.width) {
		actual.col = window_content.line_list[actual.row].length.width - 1;
	}

	return actual;
}

void wi_quit_rendering(const char _, wi_session* session) {
	(void)(_);
	session->keep_running = false;
}

void wi_quit_rendering_and_wait(const char _, wi_session* session) {
	wi_quit_rendering(_, session);
	while (session->running_render_thread) {
		/* Sleep for 10ms */
		thrd_sleep(
			&(struct timespec) { .tv_sec = 0, .tv_nsec = 1e7 },
			NULL /* No need to catch remaining time on interrupt */
		);
	}
}


void wi_free_session(wi_session* session) {
	/* Free all the windows... Yay */
	for (int i = 0; i < session->internal.capacity_rows; i++) {
		for (int j = 0; j < session->internal.amount_cols[i]; j++) {
			wi_free_window(session->windows[i][j]);
		}
		free(session->windows[i]);
	}
	free(session->windows);
	free(session->internal.amount_cols);
	free(session->internal.capacity_cols);
	free(session->keymaps);
	free(session);
}

void wi_free_window(wi_window* window) {
	free(window->internal.depending_windows);

	for (int i = 0; i < window->internal.content_grid_row_capacity; i++) {
		for (int j = 0; j < window->internal.content_grid_col_capacity[i]; j++) {
			wi_free_content(window->content_grid[i][j]);
		}
		free(window->content_grid[i]);
	}
	free(window->content_grid);
	free(window->internal.content_grid_col_capacity);
	free(window);
}

void wi_free_content(wi_content content) {
	if (content.original.string != NULL) {
		free(content.line_list);
	}
}

#undef MALLOC_ARRAY
#undef CALLOC_ARRAY
#undef REALLOC_ARRAY
