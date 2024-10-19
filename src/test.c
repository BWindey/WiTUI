#include <stdbool.h>
#include <sys/ioctl.h>
#include "../include/wiTUI.h"

#include <time.h>
#define time_it(function_call) { \
	clock_t begin = clock(); \
	function_call; \
	clock_t end = clock(); \
	printf("Time spent calling function " #function_call ": %fms\n", (double)(1000 * (end - begin)) / CLOCKS_PER_SEC); \
}

int main(void)
{
	wi_window* window01 = wi_make_window();
	wi_window* window02 = wi_make_window();

	wi_session* session = wi_make_session();

	wi_add_window_to_session(session, window01, 0);
	wi_add_window_to_session(session, window02, 0);

	window01->border.focussed_colour = "\033[94m";
	window01->border.unfocussed_colour = "\033[34m\033[2m";
	window01->title = " This is a nice title ";
	window01->footer_alignment = CENTER;

	window02->cursor_rendering = LINEBASED;

	window02->title_alignment = CENTER;
	window02->border.focussed_colour = "\033[92m";
	window02->border.unfocussed_colour = "\033[32m\033[2m";
	window02->footer_alignment = LEFT;

	wi_add_content_to_window(
		window01,
		"This is a test.\nIt contains super-cali-fragilistic-expialedotios.",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window02,
		"This is a 2nd test!\n\nIt is longer then the previous, and is here to test wrapping. No wrap on word-boundaries yet though, that is to come.\n"
		"\nIt should\nalso\nnot\ndisplay\nall\nthe content.",
		(wi_position) { 0, 0 }
	);
	window01->width = 30;
	window02->width = 40;

	session->full_screen = false;

	/*wi_render_frame(session);*/
	wi_show_session(session);

	wi_free_session_completely(session);

	return 0;
}

