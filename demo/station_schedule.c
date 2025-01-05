#include "wi_data.h"
#include "wi_functions.h"

int main(void) {
	wi_window* window_table = wi_make_window();
	wi_window* window_extra = wi_make_window();
	wi_session* session = wi_make_session();

	window_table->width = 42;
	window_extra->width = 35;

	wi_add_window_to_session(session, window_table, 0);
	wi_add_window_to_session(session, window_extra, 0);

	wi_add_content_to_window(
		window_table,
		"Destination        \u2502 Departs    \u2502 Platform\n"
		"------------------------------------------\n"
		"Antwerp-Central    \u2502 09:16      \u2502 5\n"
		"Bruges             \u2502 09:27      \u2502 6\n"
		"Ghent              \u2502 09:41      \u2502 1\n"
		"Brussels-South     \u2502 09:46      \u2502 4\n"
		"Ghent              \u2502 10:01      \u2502 1\n"
		"Antwerp-Central    \u2502 10:16      \u2502 5\n",
		(wi_position) { 0, 0 }
	);

	wi_add_content_to_window(window_extra, "", (wi_position) { 0, 0 });
	wi_add_content_to_window(window_extra, "", (wi_position) { 1, 0 });
	wi_add_content_to_window(
		window_extra,
		"This train stops in:\n"
		" - Ghent-Sint-Pieters\n"
		" - Ghent-Dampoort\n"
		" - Lokeren\n"
		" - Sint-Niklaas\n"
		" - Antwerp-Berchem\n"
		" - Antwerp-Central\n"
		"\n"
		"The first-class seats are \n"
		"in the 3th, 6th and 9th carriages.",
		(wi_position) { 2, 0 }
	);

	wi_show_session(session);
}
