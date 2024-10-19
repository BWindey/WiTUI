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

void cursor_move_vertical(int x) {
	if (x > 0) {
		printf("\033[%dA", x);
	} else if (x < 0) {
		x = -x;
		printf("\033[%dB", x);
	}
}

void cursor_move_horizontal(int y) {
	if (y > 0) {
		printf("\033[%dC", y);
	} else if (y < 0) {
		y = -y;
		printf("\033[%dD", y);
	}
}

void cursor_go_to(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

void cursor_go_to_row(int row) {
	printf("\033[%dH", row);
}

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
				occupied_width += session->windows[row][col]->width + 2;
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
			windows_to_compute[col]->_internal_rendered_width =
				distributed_width - 2;
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

void render_window_top_border(
	const wi_border border, char* title, int width,
	int horizontal_offset, char* effect
) {
	cursor_move_horizontal(horizontal_offset);
	printf("%s", effect);
	printf("%s", border.corner_top_left);

	/* Print title */
	int current_width = 0;
	while (current_width < width && title[current_width]) {
		printf("%c", title[current_width]);
		current_width++;
	}
	/* Fill rest of top-border */
	while (current_width < width) {
		printf("%s", border.side_top);
		current_width++;
	}
	printf("%s\n", border.corner_top_right);
}

void render_window_bottom_border(const wi_border border, char* footer, int width, int horizontal_offset) {
	/* Calculate some numbers to be able to right-align the footer */
	int footer_length = strlen(footer);
	int footer_padding_length = width - footer_length;

	if (footer_padding_length < 0) {
		footer_padding_length = 0;
		footer_length = width;
	}
	cursor_move_horizontal(horizontal_offset);

	/* No need to apply colour effect, because that happens at ever line-end
	 * of the window's content */
	printf("%s", border.corner_bottom_left);
	for (int _ = 0; _ < footer_padding_length; _++) {
		printf("%s", border.side_bottom);
	}

	/* Restring footer-length if necessary (can't be longer then window-width) */
	printf("%.*s", footer_length, footer);

	printf("%s\033[0m\n", border.corner_bottom_right);
}

void render_window(const wi_window* window, int horizontal_offset) {
	wi_border border = window->border;
	char* effect;
	if (window->_internal_currently_focussed) {
		effect = border.focussed_colour;
	} else {
		effect = border.unfocussed_colour;
	}

	render_window_top_border(
		border, window->title,
		window->_internal_rendered_width, horizontal_offset, effect
	);

	/* Don't forget to free this one ;-) */
	char** contents = calculate_contents(window, window->contents[0][0]);

	/* Print rows of content with border surrounding it */
	for (int i = 0; i < window->height; i++) {
		cursor_move_horizontal(horizontal_offset);
		printf("%s", border.side_left);

		for (int j = 0; j < window->_internal_rendered_width; j++) {
			/* This will need to print content,
			 * also applies colour to "closing" border */
			printf("\033[0m%c%s", contents[i][j], effect);
		}
		free(contents[i]);

		printf("%s\n", border.side_right);
	}

	free(contents);

	render_window_bottom_border(
		border, window->footer,
		window->_internal_rendered_width, horizontal_offset
	);
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
			cursor_move_vertical(window->height + 2);

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

void handle(char c, wi_session* session) {
	session->windows[session->cursor_start.row][session->cursor_start.col]->_internal_currently_focussed = false;
	/* Hardcoded values for CTRL-hjkl */
	switch (c) {
		case 8:
			session->cursor_start.col--;
			break;
		case 10:
			session->cursor_start.row++;
			break;
		case 11:
			session->cursor_start.row--;
			break;
		case 12:
			session->cursor_start.col++;
			break;
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
