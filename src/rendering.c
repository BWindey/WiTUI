#include <signal.h>		/* struct sigaction, sigaction, SIGINT */
#include <stdatomic.h>	/* atomic_bool */
#include <stdbool.h>	/* true, false */
#include <stdio.h>		/* printf() */
#include <string.h>		/* strlen() */
#include <sys/ioctl.h>	/* ioctl() */
#include <threads.h>	/* thrd_t, thrd_create, thrd_join */

#include "wiAssert.h" 	/* wiAssert() */

#include "wi_internals.h"	/* input_function() */
#include "wi_data.h"

/* This file implements wi_render_frame, wi_show_session from wi_functions.h */
#include "wi_functions.h"

typedef struct terminal_size {
	int rows;
	int cols;
} terminal_size;

static inline void clear_screen(void) {
	printf("\033[1;1H\033[2J");
}

static inline void cursor_move_up(const unsigned int x) {
	if (x > 0) {
		printf("\033[%dA", x);
	}
}

static inline void cursor_move_down(const unsigned int x) {
	if (x > 0) {
		printf("\033[%dB", x);
	}
}

static inline void cursor_move_right(const unsigned int y) {
	if (y > 0) {
		printf("\033[%dC", y);
	}
}

static inline void cursor_move_left(const unsigned int y) {
	if (y > 0) {
		printf("\033[%dD", y);
	}
}

/*
 * Move the cursor to an absolute position in the terminal.
 */
static inline void cursor_go_to(const int row, const int col) {
	printf("\033[%d;%dH", col, row);
}

/*
 * Move the cursor to an absolute row in the terminal.
 */
static inline void cursor_go_to_row(const int row) {
	printf("\033[%dH", row);
}

/*
 * Get the current terminal size as a struct containing .rows and .cols.
 * The size is expressed in amount of characters.
 *
 * @returns: struct with current terminal size
 */
terminal_size get_terminal_size(void) {
	struct winsize max;
	ioctl(0, TIOCGWINSZ, &max);
	return (terminal_size) { max.ws_row, max.ws_col };
}

/*
 * Per row, calculate the rendered width for each window.
 * This only changes when the normal width is set to -1.
 * When multiple windows have their width set to -1, the available space
 * will be distributed equally between them.
 *
 * When the previous window-size is the same as in the previous call, this
 * will do nothing and return false.
 *
 * @returns: if dimensions were re-calculated
 */
bool calculate_window_dimension(wi_session* session) {
	static terminal_size previous_size; /* static is auto initialised to 0 */
	terminal_size current_size = get_terminal_size();

	if (
		current_size.cols == previous_size.cols
		&& current_size.rows == previous_size.rows
	) {
		return false;
	}
	previous_size = current_size;

	const int available_width = current_size.cols;

	wi_window* window;

	for (int row = 0; row < session->internal.amount_rows; row++) {
		wi_window* windows_to_compute[session->internal.amount_cols[row]];
		int amount_to_compute = 0;
		int occupied_width = 0;


		/* Find windows with width -1,
		 * the others already can have their rendered width */
		for (int col = 0; col < session->internal.amount_cols[row]; col++) {
			window = session->windows[row][col];
			if (window->width == -1) {
				windows_to_compute[amount_to_compute] = window;
				amount_to_compute++;
			} else {
				window->internal.rendered_width = window->width;
				occupied_width += session->windows[row][col]->width;

				if (window->border.side_left != NULL) {
					occupied_width++;
				}
				if (window->border.side_right != NULL) {
					occupied_width++;
				}
			}
			window->internal.rendered_height = window->height;
		}

		if (amount_to_compute == 0) {
			continue;
		}

		/* Calculate how wide each window can be */
		const int width_to_distribute = available_width - occupied_width;
		const int distributed_width = width_to_distribute / amount_to_compute;
		const int left_over =
			width_to_distribute - (distributed_width * amount_to_compute);

		for (int col = 0; col < amount_to_compute; col++) {
			/* -2 because border */
			wi_window* window = windows_to_compute[col];
			window->internal.rendered_width = distributed_width;
			if (window->border.side_left != NULL) {
				window->internal.rendered_width--;
			}
			if (window->border.side_right != NULL) {
				window->internal.rendered_width--;
			}

			/* Distribute left-over among the first windows, to fill screen */
			if (col < left_over) {
				windows_to_compute[col]->internal.rendered_width++;
			}
		}
	}

	return true;
}

/*
 * Calculate the amount of characters to print on a line,
 * while preventing to wrap inside a word.
 *
 * This will make sure that `content_pointer[wrap]`
 * (`wrap` being the return value)
 * is either a newline, null byte, space or '-'.
 *
 * When there is a single word on the line that is too long to fit, the word
 * will be split at `width`.
 *
 * @returns: index on which to wrap the string
 */
int characters_until_wrap(char* content_pointer, int width) {
	int wrap = 0;
	/* TODO: */
	wrap = width;
	return wrap;
}

static inline void print_side_border(const char* border, const char* effect) {
	if (border == NULL) {
		border = "";
	}
	printf("\033[0m%s%s\033[0m", effect, border);
}

void render_content_no_wrap(const wi_window* window, const int horizontal_offset) {
	/* Extract the needed variables */
	const wi_content* content = wi_get_current_window_content(window);
	const int window_width    = window->internal.rendered_width;
	const int window_height   = window->internal.rendered_height;
	const wi_border border    = window->border;
	const char* effect = window->internal.currently_focussed
		? border.focussed_colour : border.unfocussed_colour;

	/* Cursor variables */
	wi_position cursor = window->internal.visual_cursor_position;
	bool focus_in_depending_window = false;
	for (int i = 0; i < window->internal.amount_depending; i++) {
		if (window->internal.depending_windows[i]->internal.currently_focussed) {
			focus_in_depending_window = true;
			break;
		}
	}
	bool do_line_cursor =
		(window->internal.currently_focussed || focus_in_depending_window)
		&& window->cursor_rendering == LINEBASED;
	bool do_point_cursor =
		(window->internal.currently_focussed || focus_in_depending_window)
		&& window->cursor_rendering == POINTBASED;

	const int starting_row = window->internal.content_offset_chars.row;
	int char_offset = window->internal.content_offset_chars.col;

	int cursor_line_length =
		content->line_lengths_chars[cursor.row + starting_row];

	/* Make sure that the cursor is on the content. */
	if (char_offset >= cursor_line_length) {
		char_offset = cursor_line_length - 1;
	}
	if (char_offset + cursor.col >= cursor_line_length) {
		cursor.col = cursor_line_length - char_offset - 1;
	}


	/* Variables to keep track of where I am */
	int printed_rows  = 0;
	int skipped_chars;
	int printed_chars;
	int current_byte;
	int current_line_length; /* In visual characters */
	char* current_line;

	/* Print lines with content */
	while (
		printed_rows < window_height
		&& printed_rows + starting_row < content->amount_lines
	) {
		cursor_move_right(horizontal_offset);
		print_side_border(window->border.side_left, effect);

		skipped_chars = 0;
		printed_chars = 0;
		current_byte  = 0;
		current_line  = content->lines[printed_rows + starting_row];
		current_line_length =
			content->line_lengths_chars[printed_rows + starting_row];

		/* Skip first 'char_offset' characters, but do print the ansii escape
		 * codes for text markup */
		while (skipped_chars < char_offset) {
			code_lengths char_length = char_byte_size(current_line + current_byte);
			if (current_line[current_byte] == '\033') {
				printf("%.*s", char_length.bytes, current_line + current_byte);
			}
			current_byte += char_length.bytes;
			skipped_chars += char_length.width;
		}

		/* Line cursor */
		bool line_cursor = printed_rows == cursor.row && do_line_cursor;
		if (line_cursor) {
			printf("\033[7m");
		}

		/* Print out the content */
		while (
			printed_chars < window_width
			&& printed_chars + skipped_chars < current_line_length
		) {
			/* Block cursor */
			bool point_cursor =
				printed_rows == cursor.row && printed_chars == cursor.col
				&& do_point_cursor;
			if (point_cursor) {
				printf("\033[7m");
			}

			code_lengths char_length =
				char_byte_size(current_line + current_byte);
			printf("%.*s", char_length.bytes, current_line + current_byte);
			current_byte += char_length.bytes;
			printed_chars += char_length.width;

			/* Block cursor */
			if (point_cursor) {
				printf("\033[27m"); /* Only stop cursor-effect, not the rest */
			}
		}

		/* Fill the rest of the line with emptiness */
		/* Some benchmarking showed that this was a bit faster then a loop */
		if (printed_chars < window_width) {
			printf("%*c", window_width - printed_chars, ' ');
		}

		/* No need to stop line-cursor effect, because when rendering the
		 * border, all effects are already reset. */

		print_side_border(window->border.side_right, effect);
		putchar('\n');
		printed_rows++;
	}

	/* Fill remaining height with emptiness */
	while (printed_rows < window_height) {
		cursor_move_right(horizontal_offset);
		print_side_border(window->border.side_left, effect);
		printf("%*c", window_width, ' ');
		print_side_border(window->border.side_right, effect);
		putchar('\n');

		printed_rows++;
	}
}

void render_content(const wi_window* window, const int horizontal_offset) {
	const int window_width = window->internal.rendered_width;
	const int window_height = window->internal.rendered_height;

	const wi_border border = window->border;
	const char* effect = window->internal.currently_focussed ?
		border.focussed_colour : border.unfocussed_colour;

	if (window->wrap_text) {
		/* TODO: wrapping */
		int rendered_lines = 0;
		while (rendered_lines < window_height) {
			cursor_move_right(horizontal_offset);
			print_side_border(border.side_left, effect);
			for (int _ = 0; _ < window_width; _++) {
				printf(" ");
			}
			print_side_border(border.side_right, effect);
			printf("\n");
			rendered_lines++;
		}

	} else {
		render_content_no_wrap(window, horizontal_offset);
	}
}

/*
 * Render a horizontal border with info (title/footer) in it.
 * Does not set any effect.
 * Does not jump horizontally.
 * Does not close any effect.
 * Does not place a newline.
 * Just renders the given elements, and only does that.
 *
 * The only magic happening, is the alignment.
 */
void render_horizontal_border(
	const char* left, const char* mid, const char* right,
	const wi_info_alignment alignment, const char* info, const int width
) {
	int info_length = info == NULL ? 0 : strlen(info);
	int left_pad = 0;
	int right_pad = 0;

	if (info_length > width) {
		info_length = width;
	}

	switch (alignment) {
		case LEFT:
			right_pad = width - info_length;
			break;

		case CENTER:
			right_pad = (width - info_length) / 2;
			left_pad = width - info_length - right_pad;
			break;

		case RIGHT:
			left_pad = width - info_length;
			break;
	}

	printf("%s", left);
	for (int _ = 0; _ < left_pad; _++) {
		printf("%s", mid);
	}

	/* Restrain info-length if necessary (can't be longer then window-width) */
	printf("%.*s", info_length, info);

	for (int _ = 0; _ < right_pad; _++) {
		printf("%s", mid);
	}

	printf("%s", right);
}

/*
 * Render a window at the given `horizontal_offset`.
 * This assumes that the cursor already is at the right vertical space.
 */
void render_window(const wi_window* window, const int horizontal_offset) {
	const wi_border border = window->border;
	char* effect = "";

	if (border.side_top != NULL) {
		if (window->internal.currently_focussed) {
			effect = border.focussed_colour;
		} else {
			effect = border.unfocussed_colour;
		}

		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_horizontal_border(
			border.corner_top_left, border.side_top, border.corner_top_right,
			border.title_alignment, border.title, window->internal.rendered_width
		);
		printf("\033[0m\n");
	}

	render_content(window, horizontal_offset);

	if (border.side_bottom != NULL) {
		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_horizontal_border(
			border.corner_bottom_left, border.side_bottom, border.corner_bottom_right,
			border.footer_alignment, border.footer, window->internal.rendered_width
		);
		printf("\033[0m\n");
	}
}

int wi_render_frame(wi_session* session) {
	int accumulated_row_width;
	int max_row_height;
	int accumulated_height = 0;

	wi_window* window;

	for (int row = 0; row < session->internal.amount_rows; row++) {
		accumulated_row_width = 0;
		max_row_height = 0;

		for (int col = 0; col < session->internal.amount_cols[row]; col++) {
			window = session->windows[row][col];

			render_window(window, accumulated_row_width);

			int printed_height = window->internal.rendered_height;
			if (window->border.side_top != NULL) {
				printed_height++;
			}
			if (window->border.side_bottom != NULL) {
				printed_height++;
			}
			cursor_move_up(printed_height);

			accumulated_row_width += window->internal.rendered_width + 2;
			if (printed_height > max_row_height) {
				max_row_height = printed_height;
			}
		}
		cursor_move_down(max_row_height);

		accumulated_height += max_row_height;
	}

	return accumulated_height;
}

void print_debug_cursor(wi_session* session) {
	clear_screen();
	wi_window* w = session->windows[session->focus_pos.row][session->focus_pos.col];
	wi_position cursor_offset_c = w->internal.content_offset_chars;
	wi_position cursor_visual = w->internal.visual_cursor_position;

	wi_content* content = wi_get_current_window_content(w);
	int cur_line = cursor_offset_c.row + cursor_visual.row;

	printf(
		"Offset: { %d, %d }, visual: { %d, %d }\n"
		"Content: { chars: %d, bytes: %d }\n",
		cursor_offset_c.row, cursor_offset_c.col,
		cursor_visual.row, cursor_visual.col,
		content->line_lengths_chars[cur_line],
		content->line_lengths_bytes[cur_line]
	);
}

int render_function(void* arg) {
	wi_session* session = (wi_session*) arg;
	int printed_height = 0;

	calculate_window_dimension(session);
	atomic_store(&(session->need_rerender), true);

	while (atomic_load(&(session->keep_running))) {
		bool dimensions_changed = calculate_window_dimension(session);
		if (dimensions_changed || atomic_load(&(session->need_rerender))) {
			if (session->start_clear_screen || dimensions_changed) {
				clear_screen();
			} else {
				cursor_move_up(printed_height);
			}
			/*print_debug_cursor(session);*/
			printed_height = wi_render_frame(session);
			atomic_store(&(session->need_rerender), false);
		}

		/* Sleep for 10ms */
		thrd_sleep(
			&(struct timespec) { .tv_sec = 0, .tv_nsec = 1e7 },
			NULL /* No need to catch remaining time on interrupt */
		);
	}

	return 0;
}

void handle_sigint(int _) {
	(void)(_);
	restore_terminal();
	exit(0);
}

wi_result wi_show_session(wi_session* session) {
	wi_result cursor_position = (wi_result) {
		(wi_position) { 0, 0 },
		(wi_position) { 0, 0 }
	};

	int focus_row = session->focus_pos.row;
	int focus_col = session->focus_pos.col;
	wiAssert(
		session->focus_pos.col < session->internal.amount_cols[focus_col]
		&& session->focus_pos.row < session->internal.amount_rows,
		"Can not focus on non-existing window."
	);

	/* Set starting focussed window */
	session->windows[focus_row][focus_col]->internal.currently_focussed = true;

	/* Catch ctrl+c for safety, although quick test said I don't really need it */
	struct sigaction sa = { 0 };
	sa.sa_handler = handle_sigint;
	sigaction(SIGINT, &sa, NULL);


	/* Initialise threading */
	thrd_t render_thread, input_thread;

	/* Start threads, go! */
	thrd_create(&render_thread, render_function, session);
	thrd_create(&input_thread, input_function, session);

	thrd_join(render_thread, NULL);
	thrd_join(input_thread, NULL);

	return cursor_position;
}

void wi_clear_screen_afterwards(wi_session* session) {
	for (int i = session->internal.amount_rows - 1; i >= 0; i--) {
		/* Determine highest window in the row */
		int row_height = 0;
		for (int j = 0; j < session->internal.amount_cols[i]; j++) {
			int height = session->windows[i][j]->internal.rendered_height;
			if (session->windows[i][j]->border.side_bottom != NULL) {
				height++;
			}
			if (session->windows[i][j]->border.side_top != NULL) {
				height++;
			}

			if (height > row_height) {
				row_height = height;
			}
		}

		while (row_height > 0) {
			cursor_move_up(1);
			printf("\033[2K");
			row_height--;
		}
	}
}
