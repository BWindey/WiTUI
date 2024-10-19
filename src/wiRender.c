#include <stdbool.h>	/* true, false */
#include <stdio.h>		/* printf() */
#include <string.h>		/* strlen() */
#include <sys/ioctl.h>	/* ioctl() */
#include <termios.h>	/* tcgetattr(), tcsetattr() */
#include <unistd.h>		/* read(), ICANON, ECHO, ... */

#include "../../WiTesting/wiAssert.h" 	/* wiAssert() */

/* This file implemenets wi_render_frame, wi_show_session */
#include "../include/wiTUI.h"

typedef struct terminal_size {
	int rows;
	int cols;
} terminal_size;

/* Get 1 key-press from the user */
char get_char() {
	char buf = 0;
	struct termios old = {0};
	/* Save old settings */
	wiAssert(tcgetattr(0, &old) >= 0, "tcsetattr()");

	/* Set to raw mode */
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	wiAssert(tcsetattr(0, TCSANOW, &old) >= 0, "tcsetattr ICANON");

	long read_result = read(STDIN_FILENO, &buf, 1);
	wiAssert(read_result >= 0, "Error reading key" );

	/* Set back to normal mode */
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	wiAssert(tcsetattr(0, TCSADRAIN, &old) >= 0, "tcsetattr ~ICANON");

	return buf;
}

void clear_screen() {
	printf("\033[1;1H\033[2J");
}

/*
 * Move the terminal cursor up or down.
 * Positive x means down, negative x means up.
 */
void cursor_move_vertical(int x) {
	if (x > 0) {
		printf("\033[%dA", x);
	} else if (x < 0) {
		x = -x;
		printf("\033[%dB", x);
	}
}

/*
 * Move the terminal cursor left or right.
 * Positive y means right, negative y means left.
 */
void cursor_move_horizontal(int y) {
	if (y > 0) {
		printf("\033[%dC", y);
	} else if (y < 0) {
		y = -y;
		printf("\033[%dD", y);
	}
}

/*
 * Move the cursor to an absolute position in the terminal.
 *
 * NOTE: currently unused
 */
void cursor_go_to(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

/*
 * Move the cursor to an absolute row in the terminal.
 *
 * NOTE: currently unused
 */
void cursor_go_to_row(int row) {
	printf("\033[%dH", row);
}

/*
 * Get the current terminal size as a struct containing .rows and .cols.
 * The size is expressed in amount of characters.
 *
 * @returns: struct with current terminal size
 */
terminal_size get_terminal_size() {
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
 * @returns: updated session
 */
wi_session* calculate_window_widths(wi_session* session) {
	const int available_width = get_terminal_size().cols;
	wi_window* window;

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		wi_window* windows_to_compute[session->_internal_amount_cols[row]];
		int amount_to_compute = 0;
		int occupied_width = 0;

		/* Find windows with width -1,
		 * the others already can have their rendered width */
		for (int col = 0; col < session->_internal_amount_cols[row]; col++) {
			window = session->windows[row][col];
			if (window->width == -1) {
				windows_to_compute[amount_to_compute] = window;
				amount_to_compute++;
			} else {
				/* +2 because border */
				window->_internal_rendered_width = window->width;
				occupied_width += session->windows[row][col]->width;

				if (window->border.corner_bottom_left != NULL) {
					occupied_width += 2;
				}
			}
		}

		if (amount_to_compute == 0) {
			return session;
		}

		/* Calculate how wide each window can be */
		const int width_to_distribute = available_width - occupied_width;
		const int distributed_width = width_to_distribute / amount_to_compute;
		const int left_over =
			width_to_distribute - (distributed_width * amount_to_compute);

		for (int col = 0; col < amount_to_compute; col++) {
			/* -2 because border */
			wi_window* window = windows_to_compute[col];
			window->_internal_rendered_width = distributed_width;
			if (window->border.corner_bottom_left != NULL) {
				window->_internal_rendered_width -= 2;
			}

			/* Distribute left-over among the first windows, to fill screen */
			if (col < left_over) {
				windows_to_compute[col]->_internal_rendered_width++;
			}
		}
	}

	return session;
}

/*
 * Calculate the amount of characters to print on a line,
 * while preventing to wrap inside a word.
 *
 * This will make sure that `content_pointer[wrap]`
 * (`wrap` being the return value)
 * is either a newline, nullbyte, space or '-'.
 *
 * When there is a single word on the line that is too long to fit, the word
 * will be split at `width`.
 *
 * @returns: index on which to wrap the string
 */
int characters_until_wrap(char* content_pointer, int width) {
	int wrap = 0;
	char c;

	for (int i = 0; i < width; i++) {
		c = content_pointer[i];
		if (c == '\0' || c == '\n') {
			wrap = i;
			return wrap;
		} else if (c == ' ' || c == '-') {
			if (i + 1 < width) {
				wrap = i + 1;
			} else {
				wrap = i;
			}
		}
	}

	if (wrap == 0) {
		return width;
	}

	return wrap;
}

/*
 * Calculate how a content will be rendered in the context of the given window,
 * so accounting for:
 * 	- window._internal_rendered_width/height,
 * 	- window.wrapText
 * 	- window.cursor_rendering
 * 	- window.cursor_position
 *
 * The content will (if needed) be wrapped or shifted according to the cursor
 * position, and the cursor position will be highlighted if needed.
 *
 * @returns: string-array with the contents of each line
 */
char** calculate_contents(const wi_window* window, char* content_pointer) {
	int width = window->_internal_rendered_width;
	int height = window->_internal_rendered_height;

	char** rendered_content = (char**) malloc(height * sizeof(char*));

	char filler = ' ';

	for (int current_height = 0; current_height < height; current_height++) {
		rendered_content[current_height] = (char*) malloc(width * sizeof(char));

		int chars_until_wrap = characters_until_wrap(content_pointer, width);
		for (int i = 0; i < chars_until_wrap; i++) {
			rendered_content[current_height][i] = *content_pointer;
			content_pointer++;
		}
		if (*content_pointer == '\n') {
			content_pointer++;
		}
		for (int i = chars_until_wrap; i < width; i++) {
			rendered_content[current_height][i] = filler;
		}
	}

	return rendered_content;
}

/*
 * Render a horizontal border with info (title/footer) in it.
 * Does not set any effect.
 * Does not jump horizontally.
 * Does not close any effect.
 * Does not place a newline.
 * Just renders the given elements, and only does that.
 *
 * The only magick happening, is the alignment.
 */
void render_window_border(
	const char* left, const char* mid, const char* right,
	const wi_info_alignment alignment, const char* info, const int width
) {
	int info_length = strlen(info);
	int left_pad;
	int right_pad;

	if (info_length > width) {
		info_length = width;
	}

	switch (alignment) {
		case LEFT:
			left_pad = 0;
			right_pad = width - info_length;
			break;

		case CENTER:
			right_pad = (width - info_length) / 2;
			left_pad = width - info_length - right_pad;
			break;

		case RIGHT:
			right_pad = 0;
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
void render_window(const wi_window* window, int horizontal_offset) {
	wi_border border = window->border;
	char* effect;

	if (border.corner_bottom_left != NULL) {
		if (window->_internal_currently_focussed) {
			effect = border.focussed_colour;
		} else {
			effect = border.unfocussed_colour;
		}

		cursor_move_horizontal(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_top_left, border.side_top, border.corner_top_right,
			window->title_alignment, window->title, window->_internal_rendered_width
		);
		printf("\033[0m\n");
	}

	/* Don't forget to free this one ;-) */
	char** contents = calculate_contents(window, window->contents[0][0]);

	/* Print rows of content with border surrounding it */
	for (int i = 0; i < window->_internal_rendered_height; i++) {
		cursor_move_horizontal(horizontal_offset);

		if (border.corner_bottom_left != NULL) {
			printf("%s%s\033[0m", effect, border.side_left);
		}

		if (
			window->_internal_currently_focussed
			&& window->cursor_rendering == LINEBASED
			&& window->_internal_last_cursor_position.row == i
		) {
			printf("\033[7m");
		}

		for (int j = 0; j < window->_internal_rendered_width; j++) {
			/* This will need to print content,
			 * also applies colour to "closing" border */
			if (
				window->_internal_currently_focussed
				&& window->_internal_last_cursor_position.row == i
				&& window->_internal_last_cursor_position.col == j
				&& window->cursor_rendering == POINTBASED
			) {
				printf("\033[7m%c\033[0m", contents[i][j]);
			} else {
				printf("%c", contents[i][j]);
			}
		}
		free(contents[i]);

		if (border.corner_bottom_left != NULL) {
			printf("\033[0m%s%s\033[0m", effect, border.side_right);
		}
		printf("\033[0m\n");
	}

	free(contents);

	if (border.corner_bottom_left != NULL) {
		cursor_move_horizontal(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_bottom_left, border.side_bottom, border.corner_bottom_right,
			window->footer_alignment, window->footer, window->_internal_rendered_width
		);
		printf("\033[0m\n");
	}
}

int wi_render_frame(wi_session* session) {
	int accumulated_row_width;
	int max_row_height;
	int accumulated_height = 0;

	wi_window* window;

	calculate_window_widths(session);

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		accumulated_row_width = 0;
		max_row_height = 0;

		for (int col = 0; col < session->_internal_amount_cols[row]; col++) {
			window = session->windows[row][col];

			render_window(window, accumulated_row_width);
			cursor_move_vertical(window->height);
			if (window->border.corner_bottom_left != NULL) {
				cursor_move_vertical(2);
			}

			accumulated_row_width += window->_internal_rendered_width + 2;
			if (window->height + 2 > max_row_height) {
				max_row_height = window->height + 2;
			}
		}
		cursor_move_vertical(-max_row_height);

		accumulated_height += max_row_height;
	}

	return accumulated_height;
}

/*
 * Helping function for `handle()`,
 * to make sure no invalid cursor-positions occur.
 */
void normalise_position(wi_session* session) {
	int row = session->cursor_start.row;
	int col = session->cursor_start.col;

	if (row < 0) {
		session->cursor_start.row = 0;
		row = 0;
	} else if (row >= session->_internal_amount_rows) {
		session->cursor_start.row = session->_internal_amount_rows - 1;
		row = session->_internal_amount_rows - 1;
	}

	if (col < 0) {
		session->cursor_start.col = 0;
	} else if (col >= session->_internal_amount_cols[row]) {
		session->cursor_start.col = session->_internal_amount_cols[row] - 1;
	}
}

/*
 * Handle off the key-press.
 * This can move the cursor-position between windows, and inside windows.
 */
void handle(char c, wi_session* session) {
	wi_window* focussed_window = session->windows[session->cursor_start.row][session->cursor_start.col];
	focussed_window->_internal_currently_focussed = false;

	wi_movement_keys m_keys = session->movement_keys;
	/* Hardcoded values for CTRL-hjkl */
	if (c == 8 && session->cursor_start.col > 0) {
		session->cursor_start.col--;
	} else if (c == 10 && session->cursor_start.row + 1 < session->_internal_amount_rows) {
		session->cursor_start.row++;
	} else if (c == 11 && session->cursor_start.row > 0) {
		session->cursor_start.row--;
	} else if (c == 12 && session->cursor_start.col + 1 < session->_internal_amount_cols[session->cursor_start.row]) {
		session->cursor_start.col++;
	} else if (c == m_keys.left && focussed_window->_internal_last_cursor_position.col > 0) {
		focussed_window->_internal_last_cursor_position.col--;
	} else if (c == m_keys.right && focussed_window->_internal_last_cursor_position.col + 1 < focussed_window->_internal_rendered_width) {
		focussed_window->_internal_last_cursor_position.col++;
	} else if (c == m_keys.up && focussed_window->_internal_last_cursor_position.row > 0) {
		focussed_window->_internal_last_cursor_position.row--;
	} else if (c == m_keys.down && focussed_window->_internal_last_cursor_position.row + 1 < focussed_window->_internal_rendered_height) {
		focussed_window->_internal_last_cursor_position.row++;
	}

	normalise_position(session);
	session->windows[session->cursor_start.row][session->cursor_start.col]->_internal_currently_focussed = true;
}

wi_result wi_show_session(wi_session* session) {
	wi_result cursor_position = (wi_result) {
		(wi_position) { 0, 0 },
		(wi_position) { 0, 0 }
	};

	int focus_row = session->cursor_start.row;
	int focus_col = session->cursor_start.col;
	wiAssert(
		session->cursor_start.row < session->_internal_amount_rows,
		"Can not focus on non-existing window."
	);
	wiAssert(
		session->cursor_start.col < session->_internal_amount_cols[focus_col],
		"Can not focus on non-existing window."
	);

	session->windows[focus_row][focus_col]->_internal_currently_focussed = true;

	if (session->full_screen) {
		clear_screen();
	}

	int printed_height = wi_render_frame(session);
	int c = get_char();
	while (c != session->movement_keys.quit) {
		handle(c, session);

		if (session->full_screen) {
			clear_screen();
		} else {
			cursor_move_vertical(printed_height);
		}

		printed_height = wi_render_frame(session);

		c = get_char();
	}

	return cursor_position;
}
