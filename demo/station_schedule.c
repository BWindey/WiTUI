#include "wi_data.h"
#include "wi_functions.h"

#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* exit() */

void show_and_exit(const char key, wi_session* session) {
	wi_quit_rendering_and_wait(key, session);
	wi_clear_screen_afterwards(session);

	wi_window* table = session->windows[1][0];
	wi_window* extra = session->windows[1][1];

	wi_content table_lines = wi_get_current_window_content(table);
	wi_position cur_pos = wi_get_window_cursor_pos(table);
	wi_string_view table_line_string = table_lines.line_list[cur_pos.row];
	char* table_line = table_line_string.string;

	printf("The train to ");
	while (*table_line != ' ' && *table_line != '\0') {
		putchar(*table_line);
		table_line++;
	}
	while (!(*table_line >= '0' && *table_line <= '9')) {
		table_line += wi_char_byte_size(table_line).bytes;
	}
	printf(" at ");
	while ((*table_line >= '0' && *table_line <= '9') || (*table_line == ':')) {
		putchar(*table_line);
		table_line++;
	}
	if (table_line[1] == '+') {
		table_line++;
		printf(" (");
		while (*table_line != ' ') {
			putchar(*table_line);
			table_line++;
		}
		printf(")");
	}
	printf(" departs on platform ");
	while (!(*table_line >= '0' && *table_line <= '9')) {
		table_line += wi_char_byte_size(table_line).bytes;
	}
	while (*table_line >= '0' && *table_line <= '9') {
		putchar(*table_line);
		table_line++;
	}
	putchar('\n');

	wi_content extra_content = wi_get_current_window_content(extra);
	for (int i = 0; i < extra_content.amount_lines; i++) {
		printf(
			"%.*s\n", extra_content.line_list[i].length.bytes,
			extra_content.line_list[i].string
		);
	}
}

int main(void) {
	wi_window* window_table_header = wi_make_window();
	wi_window* window_table = wi_make_window();
	wi_window* window_extra = wi_make_window();
	wi_session* session = wi_make_session(true);

	window_table_header->width = 45;
	window_table->width = 45;
	window_extra->width = 35;

	window_table_header->height = 1;
	window_table->height = 8;
	window_extra->height = 8;

	window_table_header->cursor_rendering = INVISIBLE;
	window_table->cursor_rendering = LINEBASED;

	wi_add_window_to_session(session, window_table_header, 0);
	wi_add_window_to_session(session, window_table, 1);
	wi_add_window_to_session(session, window_extra, 1);

	wi_bind_dependency(window_table, window_extra);

	wi_add_content_to_window(
		window_table_header,
		"\033[1m"
		"Destination        \033[38;2;185;39;41m│\033[39m Departs    \033[38;2;185;39;41m│\033[39m Platform\n",
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window_table,
		"Antwerp-Central    \033[38;2;185;39;41m│\033[39m 09:16      \033[38;2;185;39;41m│\033[39m 5\n"
		"Bruges             \033[38;2;185;39;41m│\033[39m 09:27 +3   \033[38;2;185;39;41m│\033[39m 6\n"
		"Ghent              \033[38;2;185;39;41m│\033[39m 09:41      \033[38;2;185;39;41m│\033[39m 1\n"
		"Brussels-South     \033[38;2;185;39;41m│\033[39m 09:46      \033[38;2;185;39;41m│\033[39m 4\n"
		"Ghent              \033[38;2;185;39;41m│\033[39m 10:01      \033[38;2;185;39;41m│\033[39m 1\n"
		"Antwerp-Central    \033[38;2;185;39;41m│\033[39m 10:16      \033[38;2;185;39;41m│\033[39m 5",
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
		(wi_position) { 0, 0 }
	);
	wi_add_content_to_window(
		window_extra,
		"This train stops in:\n"
		" - Lichtervelde\n"
		" - Bruges\n"
		"\n"
		"The first-class seats are\n"
		"in the 1st and 3rd carriages.",
		(wi_position) { 1, 0 }
	);
	wi_add_content_to_window(
		window_extra,
		"This train stops in:\n"
		" - Harelbeke\n"
		" - Waregem\n"
		" - De Pinte\n"
		" - Ghent-Sint-Pieters\n"
		"\n"
		"The first-class seats are\n"
		"in the 2nd, 4th and 6th carriages.",
		(wi_position) { 2, 0 }
	);
	wi_add_content_to_window(
		window_extra,
		"This train stops in:\n"
		" - Ghent-Sint-Pieters\n"
		" - Brussels-South\n"
		"\n"
		"The first-class seats are\n"
		"in the 3rd and 9th carriages.",
		(wi_position) { 3, 0 }
	);
	wi_add_content_to_window(
		window_extra,
		"This train stops in:\n"
		" - Waregem\n"
		" - Ghent-Sint-Pieters\n"
		"\n"
		"The first-class seats are\n"
		"in the 1st carriage.",
		(wi_position) { 4, 0 }
	);
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
		(wi_position) { 5, 0 }
	);

	/* Set all colours */
	window_table_header->border.focussed_colour = "\033[38;2;243;224;74m";
	window_table_header->border.unfocussed_colour = "\033[38;2;243;224;74m";

	window_table->border.focussed_colour = "\033[38;2;52;64;106m";
	window_table->border.unfocussed_colour = "\033[38;2;52;64;106m";

	/* Tweak borders */
	window_table_header->border.side_bottom = NULL;
	window_table->border.corner_top_left = "\033[38;2;243;224;74m┝";
	window_table->border.side_top = "━";
	window_table->border.corner_top_right = "┥";

	/* Start focus in table */
	session->focus_pos.row = 1;

	/* Prevent from going to upper window */
	wi_pop_keymap_from_session(session, 'k', CTRL);

	/* Update keymaps so 'q' quits and leaves table,
	 * and 'enter' erases table and shows the extra info for the selection. */
	wi_add_keymap_to_session(session, '\n', NONE, show_and_exit);

	wi_show_session(session);
	wi_free_session(session);
}
