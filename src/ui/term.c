#include "ui.h"
#include "torch.h"
#include "termkey.h"

#include <term.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ui_cell {
	char ch[4];
	struct ui_cell_attr attr;
};

#define UI_CELL_DEFAULT (struct ui_cell) { \
	.ch = " ", \
}

struct ui_buffer {
	size_t rows;
	size_t cols;
	struct ui_cell **buffer;
	size_t last_changed_row;
};

static struct ui_buffer ui_buffer = {0};
static TermKey *termkey = NULL;

struct term_dimensions {
	size_t rows, cols;
};

static struct term_dimensions term_dimensions(void)
{
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	return (struct term_dimensions) {
		.rows = winsize.ws_row,
		.cols = winsize.ws_col,
	};
}

static void ui_buffer_realloc(struct ui_buffer *b)
{
	/* Get new terminal dimensions. */
	struct term_dimensions new_dimensions = term_dimensions();

	/* Free excess rows. */
	for (int row = new_dimensions.rows; row < b->rows; ++row) {
		free(b->buffer[row]);
	}

	/* Resize top array of pointers. */
	b->buffer = realloc(b->buffer, new_dimensions.rows * sizeof(struct ui_cell *));

	/* Resize each row. */
	for (int row = 0; row < min(b->rows, new_dimensions.rows); ++row) {
		b->buffer[row] = realloc(b->buffer[row], new_dimensions.cols * sizeof(struct ui_cell));
	}
	for (int row = b->rows; row < new_dimensions.rows; ++row) {
		b->buffer[row] = malloc(new_dimensions.cols * sizeof(struct ui_cell));
	}

	/* Update buffer dimensions. */
	b->rows = new_dimensions.rows;
	b->cols = new_dimensions.cols;
	b->last_changed_row = -1;
}

static void term_resize_handler(int signal)
{
	ui_buffer_realloc(&ui_buffer);
	(void)signal;
}

#define terminfo_init() setupterm(getenv("TERM"), 1, NULL)
#define terminfo_print_str(cap, ...) tputs(tiparm(cap, ##__VA_ARGS__), 1, putchar)

#define CSI "\x1b["

static void set_fg_rgb(struct color color)
{
	printf(CSI"38;2;%u;%u;%um", color.r, color.g, color.b);
}

static void set_bg_rgb(struct color color)
{
	printf(CSI"48;2;%u;%u;%um", color.r, color.g, color.b);
}

void ui_init(void)
{
	terminfo_init();

	ui_buffer_realloc(&ui_buffer);
	signal(SIGWINCH, &term_resize_handler);

	setvbuf(stdout, NULL, _IOFBF, 0);

	terminfo_print_str(enter_ca_mode);
	terminfo_print_str(cursor_invisible);
	terminfo_print_str(cursor_home);
	terminfo_print_str(exit_attribute_mode);
	set_fg_rgb((struct color){0, 0, 0});
	set_bg_rgb((struct color){0, 0, 0});

	termkey = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8);
}

void ui_quit(void)
{
	setlinebuf(stdout);

	terminfo_print_str(cursor_visible);
	terminfo_print_str(exit_ca_mode);

	termkey_destroy(termkey);
}

void ui_dimensions(int *restrict rows, int *restrict cols)
{
	if (rows)
		*rows = ui_buffer.rows;
	if (cols)
		*cols = ui_buffer.cols;
}

static inline bool ui_buffer_in_bounds(int row, int col)
{
	return row >= 0 && row < ui_buffer.rows && col >= 0 && col < ui_buffer.cols;
}

static size_t utf8_codepoint_len(const char *utf8)
{
	if ((utf8[0] & 0xf0) == 0xf0) 
		return 4;
	if ((utf8[0] & 0xe0) == 0xe0)
		return 3;
	if ((utf8[0] & 0xc0) == 0xc0)
		return 2;
	return 1;
}

static struct ui_cell *ui_buffer_at(int row, int col)
{
	if (ui_buffer_in_bounds(row, col))
		return &ui_buffer.buffer[row][col];
	else
		return NULL;
}

static void utf8_copy(char *restrict dest, const char *restrict src)
{
	size_t len = utf8_codepoint_len(src);
	memcpy(dest, src, len);
	if (len != 4)
		dest[len] = '\0';
}

void ui_draw_at(int row, int col, const char *str, struct ui_cell_attr attr)
{
	size_t pos = 0;
	for (const char *ch = str; *ch; ch += utf8_codepoint_len(ch)) {
		struct ui_cell *cell = ui_buffer_at(row, col + pos);
		if (!cell)
			break;
		utf8_copy(cell->ch, ch);
		cell->attr = attr;
		pos++;
	}

	if (row > ui_buffer.last_changed_row)
		ui_buffer.last_changed_row = row;
}

static void move_cursor(int row, int col)
{
	terminfo_print_str(cursor_address, row, col);
}

static void update_attributes(struct ui_cell_attr cur, struct ui_cell_attr last)
{
	if (cur.bold != last.bold ||
	    cur.italics != last.italics ||
	    cur.underline != last.underline ||
	    cur.blink != last.blink ||
	    cur.reverse != last.reverse) {
		terminfo_print_str(exit_attribute_mode);
		set_fg_rgb(cur.fg);
		set_bg_rgb(cur.bg);
	} else {
		if (!color_equal(cur.fg, last.fg))
			set_fg_rgb(cur.fg);

		if (!color_equal(cur.bg, last.bg))
			set_bg_rgb(cur.bg);
	}

	if (cur.bold)
		terminfo_print_str(enter_bold_mode);

	if (cur.italics)
		terminfo_print_str(enter_italics_mode);

	if (cur.underline)
		terminfo_print_str(enter_underline_mode);

	if (cur.blink)
		terminfo_print_str(enter_blink_mode);

	if (cur.reverse)
		terminfo_print_str(enter_reverse_mode);
}

void ui_draw_at_incremental(int row, int col, const char *str, struct ui_cell_attr attr, int wait)
{
	move_cursor(row, col);
	update_attributes(attr, (struct ui_cell_attr) {});

	size_t pos = 0;
	for (const char *ch = str; *ch; ch += utf8_codepoint_len(ch)) {
		if (!ui_buffer_in_bounds(row, col + pos))
			return;
		char buf[4] = {0};
		utf8_copy(buf, ch);
		printf("%.4s", buf);
		fflush(stdout);
		usleep(wait * 1000);
		pos++;
	}
}

void ui_set_attr_at(int row, int col, struct ui_cell_attr attr)
{
	struct ui_cell *cell = ui_buffer_at(row, col);
	if (cell)
		cell->attr = attr;
}

void ui_clear(void)
{
	for (int row = 0; row < ui_buffer.rows; ++row)
		for (int col = 0; col < ui_buffer.cols; ++col)
			*ui_buffer_at(row, col) = UI_CELL_DEFAULT;
}

void ui_flush(void)
{
	terminfo_print_str(cursor_home);

	struct ui_cell last = UI_CELL_DEFAULT;
	for (int row = 0; row < ui_buffer.rows; ++row) {
		for (int col = 0; col < ui_buffer.cols; ++col) {
			struct ui_cell cur = *ui_buffer_at(row, col);
			update_attributes(cur.attr, last.attr);
			printf("%.4s", cur.ch);
			last = cur;
		}

		if (row == ui_buffer.last_changed_row) {
			ui_buffer.last_changed_row = 0;
			return;
		}
	}

	fflush(stdout);
}

bool ui_polling = false;

struct ui_event ui_poll_event(void)
{
	TermKeyKey key;
	struct ui_event event;

	ui_polling = true;
	while (ui_polling)
		switch (termkey_waitkey(termkey, &key)) {
		case TERMKEY_RES_KEY:
			event.key = key.code.codepoint;
			ui_polling = false;
		default:
			break;
		}

	return event;
}

#ifdef TEST

int main(void)
{
	ui_init();

	ui_clear();
	ui_draw_at(4, 4, "Test", (struct ui_cell_attr) {
		.fg = {0xe2, 0x58, 0x22},
	});
	ui_flush();

	ui_poll_event();
	ui_quit();
}

#endif
