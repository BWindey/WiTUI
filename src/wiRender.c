#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "../include/wiTUI.h"

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
	window1->width = 40;
	window1->height = 5;

	window2->border.focussed_colour = "\033[92m";
	window2->width = 20;

	window3->border.focussed_colour = "\033[91m";
	window3->width = 80;
	window3->height = 15;

	/* Proof of concept drawing */
	wi_border border = window1->border;

	/* Top border */
	printf("%s", border.focussed_colour);
	printf("%s", border.corner_top_left);
	for (int i = 0; i < window1->width; i++) {
		printf("%s", border.side_top);
	}
	printf("%s\n", border.corner_top_right);

	/* Contents (empty for now) */
	for (int i = 0; i < window1->height; i++) {
		printf("%s", border.side_left);
		for (int j = 0; j < window1->width; j++) {
			printf("\033[0m%c%s", '1', border.focussed_colour); 		/* This will need to print content */
		}
		printf("%s\n", border.side_right);
	}

	/* Bottom border */
	printf("%s", border.corner_bottom_left);
	for (int i = 0; i < window1->width; i++) {
		printf("%s", border.side_bottom);
	}
	printf("%s\033[0m\n", border.corner_bottom_right);

	/* --------------------------------------------- */

	cursor_move_vertical(window1->height + 2);
	cursor_move_horizontal(window1->width + 2);

	border = window2->border;

	/* Top border */
	printf("%s", border.focussed_colour);
	printf("%s", border.corner_top_left);
	for (int i = 0; i < window2->width; i++) {
		printf("%s", border.side_top);
	}
	printf("%s\n", border.corner_top_right);

	/* Contents (empty for now) */
	for (int i = 0; i < window2->height; i++) {
		cursor_move_horizontal(window1->width + 2);
		printf("%s", border.side_left);
		for (int j = 0; j < window2->width; j++) {
			printf("\033[0m%c%s", '2', border.focussed_colour); 		/* This will need to print content */
		}
		printf("%s\n", border.side_right);
	}

	/* Bottom border */
	cursor_move_horizontal(window1->width + 2);
	printf("%s", border.corner_bottom_left);
	for (int i = 0; i < window2->width; i++) {
		printf("%s", border.side_bottom);
	}
	printf("%s\033[0m\n", border.corner_bottom_right);

	/* --------------------------------------------- */

	border = window3->border;

	/* Top border */
	printf("%s", border.focussed_colour);
	printf("%s", border.corner_top_left);
	for (int i = 0; i < window3->width; i++) {
		printf("%s", border.side_top);
	}
	printf("%s\n", border.corner_top_right);

	/* Contents (empty for now) */
	for (int i = 0; i < window3->height; i++) {
		printf("%s", border.side_left);
		for (int j = 0; j < window3->width; j++) {
			printf("\033[0m%c%s", '3', border.focussed_colour); 		/* This will need to print content */
		}
		printf("%s\n", border.side_right);
	}

	/* Bottom border */
	printf("%s", border.corner_bottom_left);
	for (int i = 0; i < window3->width; i++) {
		printf("%s", border.side_bottom);
	}
	printf("%s\033[0m\n", border.corner_bottom_right);


	return 0;
}

