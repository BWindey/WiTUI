#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "../include/wiTUI.h"

#include <time.h>
#define time_it(function_call) { \
	clock_t begin = clock(); \
	function_call; \
	clock_t end = clock(); \
	printf("Time spent calling function " #function_call ": %fms\n", (double)(1000 * (end - begin)) / CLOCKS_PER_SEC); \
}

typedef struct terminal_size {
	int rows;
	int cols;
} terminal_size;

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

void calculate_window_widths(const wi_session* session) {

}

void render_window_top_border(const wi_border border, char* title, int width, int horizontal_offset) {
	cursor_move_horizontal(horizontal_offset);
	printf("%s", border.focussed_colour);
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

	render_window_top_border(
		border, window->title,
		window->_internal_rendered_width, horizontal_offset
	);

	/* Contents (empty for now) */
	for (int i = 0; i < window->height; i++) {
	cursor_move_horizontal(horizontal_offset);
		printf("%s", border.side_left);
		for (int j = 0; j < window->_internal_rendered_width; j++) {
			/* This will need to print content,
			 * also applies colour to "closing" border */
			printf("\033[0m%c%s", 'c', border.focussed_colour);
		}
		printf("%s\n", border.side_right);
	}

	render_window_bottom_border(
		border, window->footer,
		window->_internal_rendered_width, horizontal_offset
	);
}

void render_frame(const wi_session* session) {
	int accumulated_row_width;
	int max_row_height;

	wi_window* window;

	/*calculate_window_widths(session);*/

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		accumulated_row_width = 0;
		max_row_height = 0;

		for (int col = 0; col < session->_internal_amount_cols[row]; col++) {
			window = session->windows[row][col];

			if (window->width == -1 && col < session->_internal_amount_cols[row]) {
				window->_internal_rendered_width = get_terminal_size().cols - accumulated_row_width - 2;
			}

			render_window(window, accumulated_row_width);
			cursor_move_vertical(window->height + 2);

			accumulated_row_width += window->width + 2;
			if (window->height + 2 > max_row_height) {
				max_row_height = window->height + 2;
			}
		}

		cursor_move_vertical(-max_row_height);
	}
}

int main(void)
{
	wi_window* window1 = wi_make_window();
	wi_window* window2 = wi_make_window();
	wi_window* window3 = wi_make_window();
	wi_session* session = wi_make_session();

	wi_add_window_to_session(session, window1, 0);
	wi_add_window_to_session(session, window2, 0);
	wi_add_window_to_session(session, window3, 1);

	window1->border.focussed_colour = "\033[94m";
	free(window1->title);
	window1->title = strdup(" This is a nice title ");
	window1->width = 40;
	window1->_internal_rendered_width = 40;

	window2->border.focussed_colour = "\033[92m";
	window2->width = -1;
	window2->_internal_rendered_width = -1;

	window3->border.focussed_colour = "\033[91m";
	free(window3->title);
	window3->title = strdup(" The green window has the standard title ");
	window3->width = -1;
	window3->_internal_rendered_width = -1;
	window3->height = 15;

	time_it(render_frame(session));

	wi_free_session_completely(session);

	return 0;
}

