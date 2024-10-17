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

int main(void)
{
	wi_window* window01 = wi_make_window();
	wi_window* window02 = wi_make_window();

	wi_window* window10 = wi_make_window();
	wi_window* window11 = wi_make_window();
	wi_window* window12 = wi_make_window();

	wi_session* session = wi_make_session();

	wi_add_window_to_session(session, window01, 0);
	wi_add_window_to_session(session, window02, 0);
	wi_add_window_to_session(session, window10, 1);
	wi_add_window_to_session(session, window11, 1);
	wi_add_window_to_session(session, window12, 1);

	window01->border.focussed_colour = "\033[94m";
	window01->title = " This is a nice title ";
	window01->width = -1;

	window02->border.focussed_colour = "\033[92m";
	window02->width = -1;

	window10->border.focussed_colour = "\033[91m";
	window10->title = " The green window has the standard title ";
	window10->width = -1;

	window11->border.focussed_colour = "\033[93m";
	window11->width = -1;
	window11->border.side_bottom = "-";
	window11->border.side_top = "-";
	window11->border.side_left = "|";
	window11->border.side_right = "|";

	window12->border.focussed_colour = "\033[95m";
	window12->width = -1;

	session->full_screen = false;
	wi_show_session(session);

	wi_free_session_completely(session);

	return 0;
}

