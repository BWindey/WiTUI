#include <stdatomic.h>	/* atomic_store() */
#include <string.h>		/* strlen() */
#include <unistd.h>		/* read(), ICANON, ECHO, ... */
#include <termios.h>	/* tcgetattr(), tcsetattr() */

#include "wiTUI.h"
#include "wiAssert.h"
#include "wi_internals.h"

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
	old_terminal_settings.c_lflag &= ~ICANON;
	old_terminal_settings.c_lflag &= ~ECHO;
	old_terminal_settings.c_cc[VMIN] = 1;
	old_terminal_settings.c_cc[VTIME] = 0;
	wiAssert(
		tcsetattr(0, TCSANOW, &old_terminal_settings) >= 0,
		"tcsetattr ICANON"
	);
}

void restore_terminal(void) {
	/* Bring back cursor */
	printf("\033[?25h");

	/* Set back to normal mode */
	old_terminal_settings.c_lflag |= ICANON;
	old_terminal_settings.c_lflag |= ECHO;
	wiAssert(
		tcsetattr(0, TCSADRAIN, &old_terminal_settings) >= 0,
		"tcsetattr ~ICANON"
	);
}

/* Get 1 key-press from the user, assumes raw terminal. */
char get_char(void) {
	char buf = 0;

	long read_result = read(STDIN_FILENO, &buf, 1);
	wiAssertCallback(
		read_result >= 0,
		restore_terminal(),
		"Error reading key"
	);

	return buf;
}

/*
 * Convert a character that's in potentially a weird range, to one
 * in the range 'a-z', according to the modifier.
 * If the character is not in its expected range for the given modifier,
 * it is returned as-is.
 */
char normalised_key(char c, wi_modifier modifier) {
	switch (modifier) {
		case CTRL:
			if (c > 0 && c <= 26) {
				return c + 'a' - 1;
			}
			break;
		case SHIFT:
			if (c >= 'A' && c <= 'Z') {
				return c - 'A' + 'a';
			}
			break;
		default:
			break;
	}
	return 0;
}

/*
 * All the scroll-functions first check if we can just move the visual cursor,
 * and if that's not possible, see if we can move the offset (which will
 * actually scroll the text).
 */

/*
 * GOAL: make it so render_window_content can rely on cursor being on valid
 * position (so '\0' as last one possible)
 */
void sanitize_window_cursor_positions(wi_window* window) {
	const int visual_row = window->internal.visual_cursor_position.row;
	const int visual_col = window->internal.visual_cursor_position.col;
	const int offset_row = window->internal.content_render_offset.row;
	const int offset_col = window->internal.content_render_offset.col;

	const wi_content* content = wi_get_current_window_content(window);
	const int row_in_content = offset_row + visual_row;

	const int current_line_length = row_in_content < content->amount_lines
		? content->line_lengths[row_in_content] : 0;
	const int window_width = window->internal.rendered_width;

	if (visual_col >= current_line_length) {
		if (current_line_length != 0) {
			window->internal.visual_cursor_position.col = current_line_length - 1;
		} else {
			window->internal.visual_cursor_position.col = 0;
		}
	}
	if (offset_col + window_width >= current_line_length) {
		if (current_line_length - window_width - 1 < 0) {
			window->internal.content_render_offset.col = 0;
		} else {
			window->internal.content_render_offset.col =
				current_line_length - window_width - 1;
		}
	}
}

void wi_scroll_up(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	if (focussed_window->internal.visual_cursor_position.row > 0) {
		focussed_window->internal.visual_cursor_position.row--;
		atomic_store(&(session->cursor_has_changed), true);
	} else if (focussed_window->internal.content_render_offset.row > 0) {
		focussed_window->internal.content_render_offset.row--;
		atomic_store(&(session->cursor_has_changed), true);
	}
	sanitize_window_cursor_positions(focussed_window);
}

void wi_scroll_down(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	int fw_visual_row = focussed_window->internal.visual_cursor_position.row;
	int fw_offset_row = focussed_window->internal.content_render_offset.row;

	int fw_height = focussed_window->internal.rendered_height;
	int fw_amount_content_lines =
		wi_get_current_window_content(focussed_window)->amount_lines;

	/* Don't scroll further then the text */
	if (fw_visual_row + fw_offset_row + 1 >= fw_amount_content_lines) {
		return;
	}

	if (fw_visual_row + 1 < fw_height) {
		focussed_window->internal.visual_cursor_position.row++;
		atomic_store(&(session->cursor_has_changed), true);
	} else if (fw_offset_row + fw_height < fw_amount_content_lines) {
		focussed_window->internal.content_render_offset.row++;
		atomic_store(&(session->cursor_has_changed), true);
	}
	sanitize_window_cursor_positions(focussed_window);
}

void wi_scroll_left(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];

	const int fw_visual_col =
		focussed_window->internal.visual_cursor_position.col;
	const int fw_offset_col =
		focussed_window->internal.content_render_offset.col;
	const bool cursor_linebased = focussed_window->cursor_rendering == LINEBASED;

	if (!cursor_linebased && fw_visual_col > 0) {
		focussed_window->internal.visual_cursor_position.col--;
		atomic_store(&(session->cursor_has_changed), true);
	} else if (fw_offset_col > 0) {
		focussed_window->internal.content_render_offset.col--;
		atomic_store(&(session->cursor_has_changed), true);
	}
}

void wi_scroll_right(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	const int fw_visual_col =
		focussed_window->internal.visual_cursor_position.col;
	const int fw_offset_col =
		focussed_window->internal.content_render_offset.col;

	const int fw_width = focussed_window->internal.rendered_width;

    const wi_content* content = wi_get_current_window_content(focussed_window);
    const int current_line =
		focussed_window->internal.content_render_offset.row
		+ focussed_window->internal.visual_cursor_position.row;

    const int fw_content_line_length = strlen(content->lines[current_line]);

	const bool cursor_linebased = focussed_window->cursor_rendering == LINEBASED;

	if (
		!cursor_linebased
		&& fw_visual_col + 1 < fw_width
		&& fw_visual_col + fw_offset_col + 1 < fw_content_line_length
	) {
		focussed_window->internal.visual_cursor_position.col++;
		atomic_store(&(session->cursor_has_changed), true);
	} else if (fw_offset_col + fw_width < fw_content_line_length) {
		focussed_window->internal.content_render_offset.col++;
		atomic_store(&(session->cursor_has_changed), true);
	}
}

void un_focus(wi_session* session) {
	int cursor_row = session->cursor_pos.row;
	int cursor_col = session->cursor_pos.col;

	session->windows[cursor_row][cursor_col]
		->internal.currently_focussed = false;
}

void focus(wi_session* session) {
	int cursor_row = session->cursor_pos.row;
	int cursor_col = session->cursor_pos.col;

	session->windows[cursor_row][cursor_col]
		->internal.currently_focussed = true;
}

/*
 * Sanitize the col-number, because it can be too large when moving focus
 * from a row with 3 windows to a row with 2 windows.
 */
void sanitize_session_column_number(wi_session* session) {
	int s_cursor_row = session->cursor_pos.row;
	int s_cursor_col = session->cursor_pos.col;
	if (s_cursor_col + 1 >= session->internal.amount_cols[s_cursor_row]) {
		session->cursor_pos.col = session->internal.amount_cols[s_cursor_row] - 1;
	}
}

void wi_move_focus_up(wi_session* session) {
	if (session->cursor_pos.row > 0) {
		un_focus(session);
		session->cursor_pos.row--;
		sanitize_session_column_number(session);
		focus(session);
		atomic_store(&(session->cursor_has_changed), true);
	}
}

void wi_move_focus_down(wi_session* session) {
	if (session->cursor_pos.row + 1 < session->internal.amount_rows) {
		un_focus(session);
		session->cursor_pos.row++;
		sanitize_session_column_number(session);
		focus(session);
		atomic_store(&(session->cursor_has_changed), true);
	}
}

void wi_move_focus_left(wi_session* session) {
	if (session->cursor_pos.col > 0) {
		un_focus(session);
		session->cursor_pos.col--;
		focus(session);
		atomic_store(&(session->cursor_has_changed), true);
	}
}

void wi_move_focus_right(wi_session* session) {
	int current_col = session->cursor_pos.col;
	int max_col = session->internal.amount_cols[session->cursor_pos.row];
	if (current_col + 1 < max_col) {
		un_focus(session);
		session->cursor_pos.col++;
		focus(session);
		atomic_store(&(session->cursor_has_changed), true);
	}
}

/*
 * Handle off the key-press.
 * This can move the cursor-position between windows, and inside windows.
 */
void handle(char c, wi_session* session) {
	wi_movement_keys m_keys = session->movement_keys;

	/* First check for ALT, because that's a 2-key combo */
	if (m_keys.modifier_key == ALT && c == 27) {
		c = get_char();
		if (c == m_keys.left) {
			wi_move_focus_left(session);
		} else if (c == m_keys.right) {
			wi_move_focus_right(session);
		} else if (c == m_keys.up) {
			wi_move_focus_up(session);
		} else if (c == m_keys.down) {
			wi_move_focus_down(session);
		}

	/* Then check for normal keys without modifier */
	} else if (c == m_keys.left) {
		wi_scroll_left(session);
	} else if (c == m_keys.right) {
		wi_scroll_right(session);
	} else if (c == m_keys.up) {
		wi_scroll_up(session);
	} else if (c == m_keys.down) {
		wi_scroll_down(session);

	/* And then check for keys + modifier that produce a single char */
	} else {
		c = normalised_key(c, m_keys.modifier_key);

		if (c == m_keys.left) {
			wi_move_focus_left(session);
		} else if (c == m_keys.right) {
			wi_move_focus_right(session);
		} else if (c == m_keys.up) {
			wi_move_focus_up(session);
		} else if (c == m_keys.down) {
			wi_move_focus_down(session);
		}
	}
}

int input_function(void* arg) {
	wi_session* session = (wi_session*) arg;
	char c;

	raw_terminal();

	while (atomic_load(&(session->keep_running))) {
		c = get_char();

		if (c == session->movement_keys.quit) {
			atomic_store(&(session->keep_running), false);
		} else {
			handle(c, session);
		}
	}

	restore_terminal();

	return 0;
}
