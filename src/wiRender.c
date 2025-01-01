#include <signal.h>		/* struct sigaction, sigaction, SIGINT */
#include <stdatomic.h>	/* atomic_bool */
#include <stdbool.h>	/* true, false */
#include <stdio.h>		/* printf() */
#include <string.h>		/* strlen() */
#include <sys/ioctl.h>	/* ioctl() */
#include <termios.h>	/* tcgetattr(), tcsetattr() */
#include <threads.h>	/* thrd_t, thrd_create, thrd_join */
#include <unistd.h>		/* read(), ICANON, ECHO, ... */

#include "wiAssert.h" 	/* wiAssert() */

/* This file implements wi_render_frame, wi_show_session from wiTUI.h */
#include "wiTUI.h"

#include <time.h>

typedef struct terminal_size {
	int rows;
	int cols;
} terminal_size;

atomic_bool keep_running = true;
atomic_bool cursor_pos_changed = false;

struct termios old_terminal_settings;

void raw_terminal(void) {
	/* Hide cursor */
	printf("\033[?25l");

	old_terminal_settings = (struct termios) {0};
	/* Save old settings */
	wiAssert(tcgetattr(0, &old_terminal_settings) >= 0, "tcsetattr()");

	/* Set to raw mode */
	old_terminal_settings.c_lflag &= ~ICANON;
	old_terminal_settings.c_lflag &= ~ECHO;
	old_terminal_settings.c_cc[VMIN] = 1;
	old_terminal_settings.c_cc[VTIME] = 0;
	wiAssert(tcsetattr(0, TCSANOW, &old_terminal_settings) >= 0, "tcsetattr ICANON");
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

static inline void clear_screen(void) {
	printf("\033[1;1H\033[2J");
}

static inline void cursor_move_up(const unsigned int x) {
	if (x > 0) {
		printf("\033[%dA", x);
	}
}

static inline void cursor_move_down(const unsigned int x) {
	if (x > 0) {
		printf("\033[%dB", x);
	}
}

static inline void cursor_move_right(const unsigned int y) {
	if (y > 0) {
		printf("\033[%dC", y);
	}
}

static inline void cursor_move_left(const unsigned int y) {
	if (y > 0) {
		printf("\033[%dD", y);
	}
}

/*
 * Move the cursor to an absolute position in the terminal.
 */
static inline void cursor_go_to(const int row, const int col) {
	printf("\033[%d;%dH", col, row);
}

/*
 * Move the cursor to an absolute row in the terminal.
 */
static inline void cursor_go_to_row(const int row) {
	printf("\033[%dH", row);
}

/*
 * Get the current terminal size as a struct containing .rows and .cols.
 * The size is expressed in amount of characters.
 *
 * @returns: struct with current terminal size
 */
terminal_size get_terminal_size(void) {
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
 * When the previous window-size is the same as in the previous call, this
 * will do nothing and return false.
 *
 * @returns: if dimensions were re-calculated
 */
bool calculate_window_dimension(wi_session* session) {
	static terminal_size previous_size; /* static is auto initialised to 0 */
	terminal_size current_size = get_terminal_size();

	if (
		current_size.cols == previous_size.cols
		&& current_size.rows == previous_size.rows
	) {
		return false;
	}
	previous_size = current_size;

	const int available_width = current_size.cols;

	wi_window* window;

	for (int row = 0; row < session->internal.amount_rows; row++) {
		wi_window* windows_to_compute[session->internal.amount_cols[row]];
		int amount_to_compute = 0;
		int occupied_width = 0;


		/* Find windows with width -1,
		 * the others already can have their rendered width */
		for (int col = 0; col < session->internal.amount_cols[row]; col++) {
			window = session->windows[row][col];
			if (window->width == -1) {
				windows_to_compute[amount_to_compute] = window;
				amount_to_compute++;
			} else {
				/* +2 because border */
				window->internal.rendered_width = window->width;
				occupied_width += session->windows[row][col]->width;

				if (window->border.corner_bottom_left != NULL) {
					occupied_width += 2;
				}
			}
		}

		if (amount_to_compute == 0) {
			continue;
		}

		/* Calculate how wide each window can be */
		const int width_to_distribute = available_width - occupied_width;
		const int distributed_width = width_to_distribute / amount_to_compute;
		const int left_over =
			width_to_distribute - (distributed_width * amount_to_compute);

		for (int col = 0; col < amount_to_compute; col++) {
			/* -2 because border */
			wi_window* window = windows_to_compute[col];
			window->internal.rendered_width = distributed_width;
			if (window->border.corner_bottom_left != NULL) {
				window->internal.rendered_width -= 2;
			}

			/* Distribute left-over among the first windows, to fill screen */
			if (col < left_over) {
				windows_to_compute[col]->internal.rendered_width++;
			}
		}
	}

	return true;
}

/*
 * Calculate the amount of characters to print on a line,
 * while preventing to wrap inside a word.
 *
 * This will make sure that `content_pointer[wrap]`
 * (`wrap` being the return value)
 * is either a newline, null byte, space or '-'.
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

static inline void print_side_border(const char* border, const char* effect) {
	printf("%s%s\033[0m", effect, border);
}

void render_window_content(const wi_window* window, const int horizontal_offset) {
	const int window_width = window->internal.rendered_width;
	const int window_height = window->internal.rendered_height;

	const wi_content* content = wi_get_current_window_content(window);
	const wi_position render_offset = window->internal.content_render_offset;
	const wi_position visual_cursor_pos = window->internal.visual_cursor_position;
	const wi_cursor_rendering cursor_rendering = window->cursor_rendering;

	const wi_border border = window->border;
	const char* effect = window->internal.currently_focussed ?
		border.focussed_colour : border.unfocussed_colour;

	const bool do_line_render = window->internal.currently_focussed
		&& cursor_rendering == LINEBASED;
	const bool do_point_render = window->internal.currently_focussed
		&& cursor_rendering == POINTBASED;

	if (window->wrapText) {
	} else {
		int firs_line_show = render_offset.row;
		int last_line_show = firs_line_show + window_height;

		for (int i = firs_line_show; i < last_line_show; i++) {
			cursor_move_right(horizontal_offset);
			int col = render_offset.col;
			print_side_border(border.side_left, effect);

			/* Cursor */
			if (do_line_render && i - firs_line_show == visual_cursor_pos.row) {
				printf("\033[7m");
			}

			/* Content */
			while (i < content->amount_lines && col < (int) content->line_lengths[i] && col < window_width) {
				if (do_point_render && i - firs_line_show == visual_cursor_pos.row && col - render_offset.col == visual_cursor_pos.col) {
					printf("\033[7m");
				}
				printf("%c", content->lines[i][col]);
				if (do_point_render && i - firs_line_show == visual_cursor_pos.row && col - render_offset.col == visual_cursor_pos.col) {
					printf("\033[0m");
				}
				col++;
			}
			/* Fill rest of line */
			while (col < window_width) {
				printf(" ");
				col++;
			}

			/* Cursor */
			if (do_line_render && i - firs_line_show == visual_cursor_pos.row) {
				printf("\033[0m");
			}

			print_side_border(border.side_right, effect);
			printf("\n");
		}
	}
}

/*
 * Render a horizontal border with info (title/footer) in it.
 * Does not set any effect.
 * Does not jump horizontally.
 * Does not close any effect.
 * Does not place a newline.
 * Just renders the given elements, and only does that.
 *
 * The only magic happening, is the alignment.
 */
void render_window_border(
	const char* left, const char* mid, const char* right,
	const wi_info_alignment alignment, const char* info, const int width
) {
	int info_length = info == NULL ? 0 : strlen(info);
	int left_pad = 0;
	int right_pad = 0;

	if (info_length > width) {
		info_length = width;
	}

	switch (alignment) {
		case LEFT:
			right_pad = width - info_length;
			break;

		case CENTER:
			right_pad = (width - info_length) / 2;
			left_pad = width - info_length - right_pad;
			break;

		case RIGHT:
			left_pad = width - info_length;
			break;
	}

	printf("%s", left);
	for (int _ = 0; _ < left_pad; _++) {
		printf("%s", mid);
	}

	/* Restrain info-length if necessary (can't be longer then window-width) */
	printf("%.*s", info_length, info);

	for (int _ = 0; _ < right_pad; _++) {
		printf("%s", mid);
	}

	printf("%s", right);
}

/*
 * Render a window at the given `horizontal_offset`.
 * This assumes that the cursor already is at the right vertical space.
 */
void render_window(const wi_window* window, const int horizontal_offset) {
	const wi_border border = window->border;
	char* effect = "";

	if (border.corner_bottom_left != NULL) {
		if (window->internal.currently_focussed) {
			effect = border.focussed_colour;
		} else {
			effect = border.unfocussed_colour;
		}

		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_top_left, border.side_top, border.corner_top_right,
			border.title_alignment, border.title, window->internal.rendered_width
		);
		printf("\033[0m\n");
	}

	render_window_content(window, horizontal_offset);

	if (border.corner_bottom_left != NULL) {
		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_bottom_left, border.side_bottom, border.corner_bottom_right,
			border.footer_alignment, border.footer, window->internal.rendered_width
		);
		printf("\033[0m\n");
	}
}

void print_debug_info(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	wi_position offset = focussed_window->internal.content_render_offset;
	wi_position visual = focussed_window->internal.visual_cursor_position;

	printf("Visual: { %d, %d }, ", visual.row, visual.col);
	printf("Offset: { %d, %d }\n", offset.row, offset.col);
}

int wi_render_frame(wi_session* session) {
	int accumulated_row_width;
	int max_row_height;
	int accumulated_height = 0;

	wi_window* window;
	clear_screen();
	print_debug_info(session);

	for (int row = 0; row < session->internal.amount_rows; row++) {
		accumulated_row_width = 0;
		max_row_height = 0;

		for (int col = 0; col < session->internal.amount_cols[row]; col++) {
			window = session->windows[row][col];

			render_window(window, accumulated_row_width);

			cursor_move_up(window->internal.rendered_height);
			if (window->border.corner_bottom_left != NULL) {
				cursor_move_up(2);
			}

			accumulated_row_width += window->internal.rendered_width + 2;
			if (window->internal.rendered_height + 2 > max_row_height) {
				max_row_height = window->internal.rendered_height + 2;
			}
		}
		cursor_move_down(max_row_height);

		accumulated_height += max_row_height;
	}

	return accumulated_height;
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

wi_content* wi_get_current_window_content(const wi_window* window) {
	if (window->depends_on == NULL) {
		return window->contents[0][0];
	}
	wi_window* dep = window->depends_on;
	wi_position dep_visual_cursor_pos = dep->internal.visual_cursor_position;
	wi_position dep_content_offset = dep->internal.content_render_offset;

	int row = dep_visual_cursor_pos.row + dep_content_offset.row;
	int col = dep_visual_cursor_pos.col + dep_content_offset.col;

	if (row >= window->internal.content_rows) {
		row = window->internal.content_rows - 1;
	}
	if (col >= window->internal.content_cols[row]) {
		col = window->internal.content_cols[row] - 1;
	}

	while (col > 0 && window->contents[row][col] == NULL) {
		col--;
	}
	while (row > 0 && window->contents[row][col] == NULL) {
		row--;
	}

	return window->contents[row][col];
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
		? content->line_lengths[offset_row + visual_row] : 0;
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
		atomic_store(&cursor_pos_changed, true);
	} else if (focussed_window->internal.content_render_offset.row > 0) {
		focussed_window->internal.content_render_offset.row--;
		atomic_store(&cursor_pos_changed, true);
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

	if (fw_visual_row + 1 < fw_height) {
		focussed_window->internal.visual_cursor_position.row++;
		atomic_store(&cursor_pos_changed, true);
	} else if (fw_offset_row + fw_height < fw_amount_content_lines) {
		focussed_window->internal.content_render_offset.row++;
		atomic_store(&cursor_pos_changed, true);
	}
	sanitize_window_cursor_positions(focussed_window);
}

void wi_scroll_left(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	if (focussed_window->internal.visual_cursor_position.col > 0) {
		focussed_window->internal.visual_cursor_position.col--;
		atomic_store(&cursor_pos_changed, true);
	} else if (focussed_window->internal.content_render_offset.col > 0) {
		focussed_window->internal.content_render_offset.col--;
		atomic_store(&cursor_pos_changed, true);
	}
	sanitize_window_cursor_positions(focussed_window);
}

void wi_scroll_right(wi_session* session) {
	wi_window* focussed_window =
		session->windows[session->cursor_pos.row][session->cursor_pos.col];
	const int fw_visual_col =
		focussed_window->internal.visual_cursor_position.col;
	const int fw_offset_col =
		focussed_window->internal.content_render_offset.col;

	const int fw_width = focussed_window->internal.rendered_width;
	const int current_line =
		focussed_window->internal.content_render_offset.col
		+ focussed_window->internal.content_render_offset.col;
	const int fw_content_line_length =
		strlen(
			wi_get_current_window_content(focussed_window)->lines[current_line]
		);

	if (fw_visual_col + 1 < fw_width) {
		focussed_window->internal.visual_cursor_position.col++;
		atomic_store(&cursor_pos_changed, true);
	} else if (fw_offset_col + fw_width < fw_content_line_length) {
		focussed_window->internal.content_render_offset.col++;
		atomic_store(&cursor_pos_changed, true);
	}
	/*sanitize_window_cursor_positions(focussed_window);*/
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
		atomic_store(&cursor_pos_changed, true);
	}
}

void wi_move_focus_down(wi_session* session) {
	if (session->cursor_pos.row + 1 < session->internal.amount_rows) {
		un_focus(session);
		session->cursor_pos.row++;
		sanitize_session_column_number(session);
		focus(session);
		atomic_store(&cursor_pos_changed, true);
	}
}

void wi_move_focus_left(wi_session* session) {
	if (session->cursor_pos.col > 0) {
		un_focus(session);
		session->cursor_pos.col--;
		focus(session);
		atomic_store(&cursor_pos_changed, true);
	}
}

void wi_move_focus_right(wi_session* session) {
	int current_col = session->cursor_pos.col;
	int max_col = session->internal.amount_cols[session->cursor_pos.row];
	if (current_col + 1 < max_col) {
		un_focus(session);
		session->cursor_pos.col++;
		focus(session);
		atomic_store(&cursor_pos_changed, true);
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

int render_function(void* arg) {
	wi_session* session = *((wi_session**) arg);
	int printed_height = 0;

	while (atomic_load(&keep_running)) {
		/*
		 * TODO: when going to smaller window, cursor can be out of of the
		 * 	window, will need to manually bring it back inside. Need to decide
		 * 	if that is by bringing it to the most right column on the same line,
		 * 	or by leaving it on the same character, and thus jumping a few rows
		 * 	down.
		 * 	That latter solution seems the cleanest, but REQUIRES scrolling to
		 * 	be implemented first
		 */
		bool dimensions_changed = calculate_window_dimension(session);
		if (dimensions_changed || atomic_load(&cursor_pos_changed)) {
			if (session->full_screen || dimensions_changed) {
				clear_screen();
			} else {
				cursor_move_up(printed_height);
			}
			printed_height = wi_render_frame(session);
			atomic_store(&cursor_pos_changed, false);
		}

		/* Sleep for 10ms */
		thrd_sleep(
			&(struct timespec) { .tv_sec = 0, .tv_nsec = 1e7 },
			NULL /* No need to catch remaining time on interrupt */
		);
	}

	return 0;
}

int input_function(void* arg) {
	wi_session* session = *((wi_session**) arg);

	raw_terminal();

	char c;

	while (atomic_load(&keep_running)) {
		c = get_char();

		if (c == session->movement_keys.quit) {
			atomic_store(&keep_running, false);
		} else {
			handle(c, session);
		}
	}

	restore_terminal();

	return 0;
}

void handle_sigint(int _) {
	atomic_store(&keep_running, false);
	restore_terminal();
	exit(0);
}

wi_result wi_show_session(wi_session* session) {
	wi_result cursor_position = (wi_result) {
		(wi_position) { 0, 0 },
		(wi_position) { 0, 0 }
	};

	int focus_row = session->cursor_pos.row;
	int focus_col = session->cursor_pos.col;
	wiAssert(
		session->cursor_pos.col < session->internal.amount_cols[focus_col]
		&& session->cursor_pos.row < session->internal.amount_rows,
		"Can not focus on non-existing window."
	);

	/* Set starting focussed window */
	session->windows[focus_row][focus_col]->internal.currently_focussed = true;

	/* Catch ctrl+c for safety, although quick test said I don't really need it */
	struct sigaction sa = { 0 };
	sa.sa_handler = handle_sigint;
	sigaction(SIGINT, &sa, NULL);


	/* Initialise threading */
	thrd_t render_thread, input_thread;

	/* Start threads, go! */
	thrd_create(&render_thread, render_function, &session);
	thrd_create(&input_thread, input_function, &session);

	thrd_join(render_thread, NULL);
	thrd_join(input_thread, NULL);

	return cursor_position;
}
