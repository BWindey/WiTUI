#include <stdbool.h>	/* true, false */

#include "wiTUI.h"

int main(void)
{
	wi_window* window01 = wi_make_window();
	wi_window* window02 = wi_make_window();
	wi_window* window11 = wi_make_window();
	wi_window* window12 = wi_make_window();
	wi_window* window13 = wi_make_window();
	wi_window* window20 = wi_make_window();

	wi_session* session = wi_make_session();

	window02->wrap_text = true;

	/*window01->border = (wi_border) { 0 };*/
	window01->border.focussed_colour = "\033[94m";
	window01->border.unfocussed_colour = "\033[34m\033[2m";
	window01->border.title = " This is a nice title ";
	window01->border.footer_alignment = CENTER;

	window02->cursor_rendering = LINEBASED;
	window20->cursor_rendering = LINEBASED;
	window12->cursor_rendering = LINEBASED;

	window02->border.title = NULL;
	window02->border.title_alignment = CENTER;
	/*window02->border.focussed_colour = "\033[92m";*/
	window02->border.focussed_colour = "\033[38;2m";
	window02->border.unfocussed_colour = "\033[32m\033[2m";
	window02->border.footer_alignment = LEFT;

	window11->border.focussed_colour = "\033[91m";
	window11->border.unfocussed_colour = "\033[31m\033[2m";
	window12->border.focussed_colour = "\033[95m";
	window12->border.unfocussed_colour = "\033[35m\033[2m";
	window13->border.focussed_colour = "\033[97m";
	window13->border.unfocussed_colour = "\033[37m\033[2m";

	window20->border.focussed_colour = "\033[93m";
	window20->border.unfocussed_colour = "\033[33m\033[2m";

	wi_add_content_to_window(
		window01,
		"This is a test.\nIt contains super-cali-fragilistic-expialedotios.",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window02,
		"This is a 2nd test!\n\nIt is longer then the previous, and is here to test wrapping. No wrap on word-boundaries yet though, that is to come.\n"
		"\nIt should\nalso\nnot\ndisplay\nall\nthe content.\n"
		"It actually did, so now I have to add more\n"
		"lines to this little window\n"
		"so I can demonstrate that scrolling\n"
		"actually works!.\n"
		"\n"
		"\n"
		"This is the last line =)",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window02,
		"This is the special content of the 2nd window, visible when you're on the second row in window 1.\n"
		"At least that's what should happend...",
		(wi_position) { 1, 0 }
	);
	window02->depends_on = window01;
	wi_add_content_to_window(
		window11,
		"Hello from window11! This is the first window on the first row.\n\n"
		"Hi again from window11!",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window12,
		"\n\nThis is the most middle window of them all!",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window13,
		"Hmmmm, coming up with contents for these windows is a bit tedious...",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window20,
		"This is the last window. It should be very very long! The whole terminal "
		"width, actually. So I'm writing this long sentence, that hopefully will "
		"get wrapped? Maybe, maybe not.",
		(wi_position) { 0, 0 }
	);

	window01->width = 30;
	window02->width = 40;

	window11->width = -1;
	window12->width = 30;
	window13->width = -1;

	window11->height = 5;
	window12->height = 5;
	window13->height = 5;

	window20->width = -1;
	window20->height = 3;

	session->start_clear_screen = false;

	wi_add_window_to_session(session, window01, 0);
	wi_add_window_to_session(session, window02, 0);
	wi_add_window_to_session(session, window11, 1);
	wi_add_window_to_session(session, window12, 1);
	wi_add_window_to_session(session, window13, 1);
	wi_add_window_to_session(session, window20, 2);

	wi_add_keymap_to_session(session, 'e', ALT, wi_scroll_right);

	/*wi_render_frame(session);*/
	wi_show_session(session);

	wi_free_session_completely(session);

	return 0;
}

