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

static inline void cursor_move_up(unsigned int x) {
	if (x > 0) {
		printf("\033[%dA", x);
	}
}

static inline void cursor_move_down(unsigned int x) {
	if (x > 0) {
		printf("\033[%dB", x);
	}
}

static inline void cursor_move_right(unsigned int y) {
	if (y > 0) {
		printf("\033[%dC", y);
	}
}

static inline void cursor_move_left(unsigned int y) {
	if (y > 0) {
		printf("\033[%dD", y);
	}
}

/*
 * Move the cursor to an absolute position in the terminal.
 *
 * NOTE: currently unused
 */
void cursor_go_to(int row, int col) {
	printf("\033[%d;%dH", col, row);
}

/*
 * Move the cursor to an absolute row in the terminal.
 *
 * NOTE: currently unused
 */
void cursor_go_to_row(int row) {
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

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		wi_window* windows_to_compute[session->_internal_amount_cols[row]];
		int amount_to_compute = 0;
		int occupied_width = 0;


		/* Find windows with width -1,
		 * the others already can have their rendered width */
		for (int col = 0; col < session->_internal_amount_cols[row]; col++) {
			window = session->windows[row][col];
			if (window->width == -1) {
				windows_to_compute[amount_to_compute] = window;
				amount_to_compute++;
			} else {
				/* +2 because border */
				window->_internal_rendered_width = window->width;
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
			window->_internal_rendered_width = distributed_width;
			if (window->border.corner_bottom_left != NULL) {
				window->_internal_rendered_width -= 2;
			}

			/* Distribute left-over among the first windows, to fill screen */
			if (col < left_over) {
				windows_to_compute[col]->_internal_rendered_width++;
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

/*
 * Calculate how a content will be rendered in the context of the given window,
 * so accounting for:
 * 	- window._internal_rendered_width/height,
 * 	- window.wrapText
 * 	- window.cursor_rendering
 * 	- window.cursor_position
 *
 * The content will (if needed) be wrapped or shifted according to the cursor
 * position, and the cursor position will be highlighted if needed.
 *
 * The returned char** is allocated on the heap, and needs to be manually freed.
 * Each element (row) has to be freed, and the array itself too.
 *
 * @returns: string-array with the contents of each line
 */
char** calculate_contents(
	const wi_window* window,
	char* content_pointer,
	wi_cursor_rendering cursor_rendering
) {
	const int width = window->_internal_rendered_width;
	const int height = window->_internal_rendered_height;

	const char filler = ' ';

	const char cursor_on[] = "\033[7m";
	const char cursor_off[] = "\033[0m";
	const size_t cursor_on_length = strlen(cursor_on);
	const size_t cursor_off_length = strlen(cursor_off);

	const bool do_cursor_render = window->_internal_currently_focussed
		&& cursor_rendering != INVISIBLE;
	const bool do_line_render = window->_internal_currently_focussed
		&& cursor_rendering == LINEBASED;
	const bool do_point_render = window->_internal_currently_focussed
		&& cursor_rendering == POINTBASED;

	const int cursor_row = window->_internal_last_cursor_position.row;
	const int cursor_col = window->_internal_last_cursor_position.col;

	size_t amount_to_alloc;
	int offset;

	char** rendered_content = (char**) malloc(height * sizeof(char*));

	for (int current_height = 0; current_height < height; current_height++) {
		offset = 0;

		amount_to_alloc = width * sizeof(char) + 1; /* +1 for '\0' */
		if (do_cursor_render && current_height == cursor_row) {
			amount_to_alloc += cursor_on_length + cursor_off_length;
		}
		rendered_content[current_height] = (char*) malloc(amount_to_alloc);

		if (do_line_render && current_height == cursor_row) {
			/* memcpy instead of strcpy because we know the length */
			memcpy(rendered_content[current_height], cursor_on, cursor_on_length);
			offset = cursor_on_length;
		}

		int chars_until_wrap = characters_until_wrap(content_pointer, width);

		for (int i = 0; i < chars_until_wrap; i++) {
			/* Render cursor-point if needed */
			bool do_cursor_render_now =
				do_point_render && current_height == cursor_row
				&& (i == cursor_col || i - 1 == cursor_col);

			if (do_cursor_render_now) {
				const char* effect = i == cursor_col ? cursor_on : cursor_off;
				const size_t jump = i == cursor_col ? cursor_on_length : cursor_off_length;

			/* memcpy instead of strcpy because we know the length */
				memcpy(rendered_content[current_height] + offset + i, effect, jump);
				offset += jump;
			}

			rendered_content[current_height][offset + i] = *content_pointer;
			content_pointer++;
		}
		if (*content_pointer == '\n') {
			content_pointer++;
		}
		for (int i = chars_until_wrap; i < width; i++) {
			/* Render cursor-point if needed */
			bool do_cursor_render_now =
				do_point_render && current_height == cursor_row
				&& (i == cursor_col || i - 1 == cursor_col);

			if (do_cursor_render_now) {
				const char* effect = i == cursor_col ? cursor_on : cursor_off;
				const size_t jump = i == cursor_col ? cursor_on_length : cursor_off_length;

				memcpy(rendered_content[current_height] + offset + i, effect, jump);
				offset += jump;
			}

			rendered_content[current_height][offset + i] = filler;
		}

		if (do_line_render && current_height == cursor_row) {
			memcpy(rendered_content[current_height] + offset + width, cursor_off, cursor_off_length);
			offset += cursor_off_length;
		}

		rendered_content[current_height][offset + width] = '\0';
	}

	return rendered_content;
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
void render_window(const wi_window* window, int horizontal_offset) {
	wi_border border = window->border;
	char* effect = "";

	if (border.corner_bottom_left != NULL) {
		if (window->_internal_currently_focussed) {
			effect = border.focussed_colour;
		} else {
			effect = border.unfocussed_colour;
		}

		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_top_left, border.side_top, border.corner_top_right,
			border.title_alignment, border.title, window->_internal_rendered_width
		);
		printf("\033[0m\n");
	}

	/* Don't forget to free this one ;-) */
	char** contents = calculate_contents(window, window->contents[0][0], window->cursor_rendering);

	/* Print rows of content with border surrounding it */
	for (int i = 0; i < window->_internal_rendered_height; i++) {
		cursor_move_right(horizontal_offset);

		if (border.corner_bottom_left != NULL) {
			printf("%s%s\033[0m", effect, border.side_left);
		}

		printf("%s", contents[i]);
		free(contents[i]);

		if (border.corner_bottom_left != NULL) {
			printf("\033[0m%s%s\033[0m", effect, border.side_right);
		}
		printf("\033[0m\n");
	}

	free(contents);

	if (border.corner_bottom_left != NULL) {
		cursor_move_right(horizontal_offset);
		printf("%s", effect);
		render_window_border(
			border.corner_bottom_left, border.side_bottom, border.corner_bottom_right,
			border.footer_alignment, border.footer, window->_internal_rendered_width
		);
		printf("\033[0m\n");
	}
}

int wi_render_frame(wi_session* session) {
	int accumulated_row_width;
	int max_row_height;
	int accumulated_height = 0;

	wi_window* window;

	for (int row = 0; row < session->_internal_amount_rows; row++) {
		accumulated_row_width = 0;
		max_row_height = 0;

		for (int col = 0; col < session->_internal_amount_cols[row]; col++) {
			window = session->windows[row][col];

			render_window(window, accumulated_row_width);

			cursor_move_up(window->_internal_rendered_height);
			if (window->border.corner_bottom_left != NULL) {
				cursor_move_up(2);
			}

			accumulated_row_width += window->_internal_rendered_width + 2;
			if (window->_internal_rendered_height + 2 > max_row_height) {
				max_row_height = window->_internal_rendered_height + 2;
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

/*
 * Handle off the key-press.
 * This can move the cursor-position between windows, and inside windows.
 */
void handle(char c, wi_session* session) {
	wi_window* focussed_window = session->windows[session->cursor_start.row][session->cursor_start.col];
	focussed_window->_internal_currently_focussed = false;

	wi_movement_keys m_keys = session->movement_keys;

	/* First check for ALT, because that's a 2-key combo */
	if (m_keys.modifier_key == ALT && c == 27) {
		c = get_char();
		if (c == m_keys.left && session->cursor_start.col > 0) {
			session->cursor_start.col--;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.right && session->cursor_start.col + 1 < session->_internal_amount_cols[session->cursor_start.row]) {
			session->cursor_start.col++;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.up && session->cursor_start.row > 0) {
			session->cursor_start.row--;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.down && session->cursor_start.row + 1 < session->_internal_amount_rows) {
			session->cursor_start.row++;
			atomic_store(&cursor_pos_changed, true);
		}

		/* Then check for normal keys without modifier */
	} else if (c == m_keys.left && focussed_window->_internal_last_cursor_position.col > 0) {
		focussed_window->_internal_last_cursor_position.col--;
		atomic_store(&cursor_pos_changed, true);
	} else if (c == m_keys.right && focussed_window->_internal_last_cursor_position.col + 1 < focussed_window->_internal_rendered_width) {
		focussed_window->_internal_last_cursor_position.col++;
		atomic_store(&cursor_pos_changed, true);
	} else if (c == m_keys.up && focussed_window->_internal_last_cursor_position.row > 0) {
		focussed_window->_internal_last_cursor_position.row--;
		atomic_store(&cursor_pos_changed, true);
	} else if (c == m_keys.down && focussed_window->_internal_last_cursor_position.row + 1 < focussed_window->_internal_rendered_height) {
		focussed_window->_internal_last_cursor_position.row++;
		atomic_store(&cursor_pos_changed, true);

		/* And then check for keys + modifier that produce a single char */
	} else {
		c = normalised_key(c, m_keys.modifier_key);

		if (c == m_keys.left && session->cursor_start.col > 0) {
			session->cursor_start.col--;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.right && session->cursor_start.col + 1 < session->_internal_amount_cols[session->cursor_start.row]) {
			session->cursor_start.col++;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.up && session->cursor_start.row > 0) {
			session->cursor_start.row--;
			atomic_store(&cursor_pos_changed, true);
		} else if (c == m_keys.down && session->cursor_start.row + 1 < session->_internal_amount_rows) {
			session->cursor_start.row++;
			atomic_store(&cursor_pos_changed, true);
		}
	}

	/* Sanitize the col-number, because it can be too large now */
	if (session->cursor_start.col + 1 >= session->_internal_amount_cols[session->cursor_start.row]) {
		session->cursor_start.col = session->_internal_amount_cols[session->cursor_start.row] - 1;
	}

	session->windows[session->cursor_start.row][session->cursor_start.col]->_internal_currently_focussed = true;
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

	int focus_row = session->cursor_start.row;
	int focus_col = session->cursor_start.col;
	wiAssert(
		session->cursor_start.col < session->_internal_amount_cols[focus_col]
		&& session->cursor_start.row < session->_internal_amount_rows,
		"Can not focus on non-existing window."
	);

	/* Set starting focussed window */
	session->windows[focus_row][focus_col]->_internal_currently_focussed = true;

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
