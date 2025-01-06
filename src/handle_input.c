#include <stdatomic.h>	/* atomic_store() */
#include <threads.h>	/* thrd_sleep() */
#include <unistd.h>		/* read(), ICANON, ECHO, ... */
#include <termios.h>	/* tcgetattr(), tcsetattr() */
#include <fcntl.h>		/* fcntl(), F_GETFLS, O_NONBLOCK */
#include <errno.h>		/* errno, EAGAIN, EWOULDBLOCK */

#include "wiAssert.h"
#include "wi_functions.h"
#include "wi_internals.h"

#define WI_UNUSED(x) (void)(x)

struct termios old_terminal_settings;

void raw_terminal(void) {
	/* Hide cursor */
	printf("\033[?25l");

	old_terminal_settings = (struct termios) {0};
	/* Save old settings */
	wiAssert(
		tcgetattr(0, &old_terminal_settings) >= 0,
		"tcsetattr()"
	);

	/* Set to raw mode */
	struct termios new_settings = old_terminal_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_cc[VMIN] = 0;
	new_settings.c_cc[VTIME] = 0;
	wiAssert(
		tcsetattr(0, TCSANOW, &new_settings) >= 0,
		"tcsetattr ICANON"
	);

	/* Set stdin to non-blocking mode */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    wiAssert(flags >= 0, "fcntl F_GETFL failed");
    wiAssert(
		fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) >= 0,
		"fcntl F_SETFL failed"
	);
}

void restore_terminal(void) {
	/* Bring back cursor */
	printf("\033[?25h");

	/* Set back to normal mode */
	wiAssert(
		tcsetattr(0, TCSADRAIN, &old_terminal_settings) >= 0,
		"tcsetattr ~ICANON"
	);

	/* Set stdin back to blocking mode */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    wiAssert(flags >= 0, "fcntl F_GETFL failed");
    wiAssert(
		fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) >= 0,
		"fcntl F_SETFL failed"
	);
}

char wi_get_char(void) {
	char buf = 0;

	long read_result = read(STDIN_FILENO, &buf, 1);
	if (read_result < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* No character is available */
			return -1;
		} else {
			/* Other read error */
			wiAssertCallback(0, restore_terminal(), "Error reading key");
		}
	} else if (read_result == 0) {
		/* EOF reached (unlikely in interactive mode) */
		return -1;
	}

	return buf;
}

/*
 * Convert a character that's in potentially a weird range, to one
 * in the range 'a-z', according to the modifier.
 * If the character is not in its expected range for the given modifier,
 * it is returned as-is.
 */
char convert_key(wi_keymap keymap) {
	switch (keymap.modifier) {
		case CTRL:
			return keymap.key - 'a' + 1;
			break;
		case SHIFT:
			return keymap.key - 'a' + 'A';
			break;
		case NONE:
			return keymap.key;
			break;
		default:
			break;
	}
	return -2; /* IMPORTANT: this needs to be != -1 */
}

/*
 * All the scroll-functions first check if we can just move the visual cursor,
 * and if that's not possible, see if we can move the offset (which will
 * actually scroll the text).
 */

void wi_scroll_up(const char _, wi_session* session) {
	WI_UNUSED(_);
	wi_window* focussed_window =
		session->windows[session->focus_pos.row][session->focus_pos.col];
	if (focussed_window->internal.visual_cursor_position.row > 0) {
		focussed_window->internal.visual_cursor_position.row--;
		atomic_store(&(session->need_rerender), true);
	} else if (focussed_window->internal.content_offset_chars.row > 0) {
		focussed_window->internal.content_offset_chars.row--;
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_scroll_down(const char _, wi_session* session) {
	WI_UNUSED(_);
	wi_window* focussed_window =
		session->windows[session->focus_pos.row][session->focus_pos.col];

	/* Aliasses for these loooooong names */
	int* visual_row = &focussed_window->internal.visual_cursor_position.row;
	int* offset_row = &focussed_window->internal.content_offset_chars.row;

	const int fw_height = focussed_window->internal.rendered_height;
	const int fw_amount_content_lines =
		wi_get_current_window_content(focussed_window)->amount_lines;

	/* Don't scroll further then the text */
	if (*visual_row + *offset_row + 1 >= fw_amount_content_lines) {
		return;
	}

	if (*visual_row + 1 < fw_height) {
		(*visual_row)++;
		atomic_store(&(session->need_rerender), true);
	} else if (*offset_row + fw_height < fw_amount_content_lines) {
		(*offset_row)++;
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_scroll_left(const char _, wi_session* session) {
	WI_UNUSED(_);
	wi_window* focussed_window =
		session->windows[session->focus_pos.row][session->focus_pos.col];

	if (
		focussed_window->wrap_text
		&& focussed_window->cursor_rendering == LINEBASED) {
		return;
	}

	/* Some aliasses for these loooong names */
	int* visual_col = &focussed_window->internal.visual_cursor_position.col;
	int* offset_c_col = &focussed_window->internal.content_offset_chars.col;

	const bool cursor_linebased = focussed_window->cursor_rendering == LINEBASED;

	const wi_content* content = wi_get_current_window_content(focussed_window);
	const int current_line =
		focussed_window->internal.content_offset_chars.row
		+ focussed_window->internal.visual_cursor_position.row;
	const int line_length_c = content->line_lengths_chars[current_line];

	if (*offset_c_col >= line_length_c) {
		*offset_c_col = line_length_c - 1;
		*visual_col = 0;
		atomic_store(&(session->need_rerender), true);
	} else if (*offset_c_col + *visual_col >= line_length_c) {
		*visual_col = line_length_c - *offset_c_col - 1;
		atomic_store(&(session->need_rerender), true);
	}


	if (!cursor_linebased && *visual_col > 0) {
		(*visual_col)--;
		atomic_store(&(session->need_rerender), true);
	} else if (*offset_c_col > 0) {
		/* Move to actual start of codepoint instead of byte in the middle */
		(*offset_c_col)--;
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_scroll_right(const char _, wi_session* session) {
	WI_UNUSED(_);
	wi_window* focussed_window =
		session->windows[session->focus_pos.row][session->focus_pos.col];

	if (focussed_window->wrap_text && focussed_window->cursor_rendering == LINEBASED) {
		return;
	}

	/* Aliasses for these loooooooong names */
	int* visual_col = &focussed_window->internal.visual_cursor_position.col;
	int* offset_c_col = &focussed_window->internal.content_offset_chars.col;

	const int fw_width = focussed_window->internal.rendered_width;

    const wi_content* content = wi_get_current_window_content(focussed_window);
    const int current_line =
		focussed_window->internal.content_offset_chars.row
		+ focussed_window->internal.visual_cursor_position.row;

    const int line_length_c = content->line_lengths_chars[current_line];

	const bool cursor_linebased = focussed_window->cursor_rendering == LINEBASED;

	if (*offset_c_col >= line_length_c) {
		*offset_c_col = line_length_c - 1;
		*visual_col = 0;
		atomic_store(&(session->need_rerender), true);
	} else if (*offset_c_col + *visual_col >= line_length_c) {
		*visual_col = line_length_c - *offset_c_col - 1;
		atomic_store(&(session->need_rerender), true);
	}

	if (
		!cursor_linebased
		&& *visual_col + 1 < fw_width
		&& *visual_col + *offset_c_col + 1 < line_length_c
	) {
		(*visual_col)++;
		atomic_store(&(session->need_rerender), true);
	} else if (*offset_c_col + fw_width < line_length_c) {
		(*offset_c_col)++;
		atomic_store(&(session->need_rerender), true);
	}
}

void un_focus(wi_session* session) {
	int cursor_row = session->focus_pos.row;
	int cursor_col = session->focus_pos.col;

	session->windows[cursor_row][cursor_col]
		->internal.currently_focussed = false;
}

void focus(wi_session* session) {
	int cursor_row = session->focus_pos.row;
	int cursor_col = session->focus_pos.col;

	session->windows[cursor_row][cursor_col]
		->internal.currently_focussed = true;
}

/*
 * Sanitize the col-number, because it can be too large when moving focus
 * from a row with 3 windows to a row with 2 windows.
 */
void sanitize_session_column_number(wi_session* session) {
	int s_cursor_row = session->focus_pos.row;
	int s_cursor_col = session->focus_pos.col;
	if (s_cursor_col + 1 >= session->internal.amount_cols[s_cursor_row]) {
		session->focus_pos.col = session->internal.amount_cols[s_cursor_row] - 1;
	}
}

void wi_move_focus_up(const char _, wi_session* session) {
	WI_UNUSED(_);
	if (session->focus_pos.row > 0) {
		un_focus(session);
		session->focus_pos.row--;
		sanitize_session_column_number(session);
		focus(session);
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_move_focus_down(const char _, wi_session* session) {
	WI_UNUSED(_);
	if (session->focus_pos.row + 1 < session->internal.amount_rows) {
		un_focus(session);
		session->focus_pos.row++;
		sanitize_session_column_number(session);
		focus(session);
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_move_focus_left(const char _, wi_session* session) {
	WI_UNUSED(_);
	if (session->focus_pos.col > 0) {
		un_focus(session);
		session->focus_pos.col--;
		focus(session);
		atomic_store(&(session->need_rerender), true);
	}
}

void wi_move_focus_right(const char _, wi_session* session) {
	WI_UNUSED(_);
	int current_col = session->focus_pos.col;
	int max_col = session->internal.amount_cols[session->focus_pos.row];
	if (current_col + 1 < max_col) {
		un_focus(session);
		session->focus_pos.col++;
		focus(session);
		atomic_store(&(session->need_rerender), true);
	}
}


int input_function(void* arg) {
	wi_session* session = (wi_session*) arg;
	wi_keymap* key_maps = session->keymaps;
	int amount_maps = session->internal.amount_keymaps;

	char c;

	raw_terminal();

	while (atomic_load(&(session->keep_running))) {
		c = wi_get_char();
		if (c > 0) {
			bool alt_mod = false;

			if (c == 27) {
				c = wi_get_char();
				alt_mod = true;
			}


			for (int i = 0; i < amount_maps; i++) {
				if (key_maps[i].callback == NULL) {
					continue;
				}
				if (
					alt_mod && key_maps[i].modifier == ALT
					&& key_maps[i].key == c
				) {
					key_maps[i].callback(c, session);
				} else if (c == convert_key(key_maps[i])) {
					key_maps[i].callback(c, session);
				}
			}
		}

		/* Sleep for 10ms */
		thrd_sleep(
			&(struct timespec) { .tv_sec = 0, .tv_nsec = 1e7 },
			NULL /* No need to catch remaining time on interrupt */
		);
	}

	restore_terminal();

	return 0;
}
