#include "ui.h"
#include "torch.h"
#include "termkey.h"

#include <unibilium.h>
#include <sys/ioctl.h>
#include <unistd.h>
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
	bool changed;
};

#define UI_CELL_DEFAULT (struct ui_cell) { \
	.ch = " ", \
};

struct ui_buffer {
	size_t lines;
	size_t cols;
	struct ui_cell **buffer;
	size_t last_changed_line;
};

static struct ui_buffer ui_buffer = {0};
static unibi_term *unibi = NULL;
static TermKey *termkey = NULL;

struct term_dimensions {
	size_t lines, cols;
};

static struct term_dimensions term_dimensions(void)
{
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	return (struct term_dimensions) {
		.lines = winsize.ws_row,
		.cols = winsize.ws_col,
	};
}

static void ui_buffer_realloc(struct ui_buffer *b)
{
	/* Get new terminal dimensions. */
	struct term_dimensions new_dimensions = term_dimensions();

	/* Resize top array of pointers. */
	b->buffer = realloc(b->buffer, new_dimensions.lines * sizeof(struct ui_cell *));

	/* Resize existing lines. */
	for (int line = 0; line < min(b->lines, new_dimensions.lines); ++line) {
		b->buffer[line] = realloc(b->buffer[line], new_dimensions.cols * sizeof(struct ui_cell));
	}

	/* Allocate new lines. */
	for (int line = b->lines; line < new_dimensions.lines; ++line) {
		b->buffer[line] = calloc(new_dimensions.cols, sizeof(struct ui_cell));
	}

	/* Update buffer dimensions. */
	b->lines = new_dimensions.lines;
	b->cols = new_dimensions.cols;
	b->last_changed_line = -1;
}

static void term_resize_handler(int signal)
{
	ui_buffer_realloc(&ui_buffer);
	(void)signal;
}

static void unibi_print_str(unibi_term *unibi, enum unibi_string s, unibi_var_t param[9])
{
	const char *fmt = unibi_get_str(unibi, s);
	size_t needed = unibi_run(fmt, param, NULL, 0) + 1;
	char *buf = malloc(needed);
	buf[needed - 1] = '\0';

	unibi_run(fmt, param, buf, needed);
	fputs(buf, stdout);
}

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
	ui_buffer_realloc(&ui_buffer);
	signal(SIGWINCH, &term_resize_handler);

	setvbuf(stdout, NULL, _IOFBF, 0);

	const char *term = getenv("TERM");
	unibi = unibi_from_term(term);

	unibi_print_str(unibi, unibi_enter_ca_mode, NULL);
	unibi_print_str(unibi, unibi_cursor_invisible, NULL);
	unibi_print_str(unibi, unibi_cursor_home, NULL);
	unibi_print_str(unibi, unibi_exit_attribute_mode, NULL);
	set_fg_rgb((struct color){0, 0, 0});
	set_bg_rgb((struct color){0, 0, 0});

	termkey = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8);
}

void ui_quit(void)
{
	setlinebuf(stdout);

	unibi_print_str(unibi, unibi_cursor_visible, NULL);
	unibi_print_str(unibi, unibi_exit_ca_mode, NULL);

	unibi_destroy(unibi);
	termkey_destroy(termkey);
}

void ui_dimensions(int *restrict lines, int *restrict cols)
{
	if (lines)
		*lines = ui_buffer.lines;
	if (cols)
		*cols = ui_buffer.cols;
}

static inline bool ui_buffer_in_bounds(int line, int col)
{
	return line >= 0 && line < ui_buffer.lines && col >= 0 && col < ui_buffer.cols;
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

static struct ui_cell *ui_buffer_at(int line, int col)
{
	if (ui_buffer_in_bounds(line, col))
		return &ui_buffer.buffer[line][col];
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

void ui_draw_at(int line, int col, const char *str, struct ui_cell_attr attr)
{
	if (!ui_buffer_in_bounds(line, col))
		return;

	for (const char *ch = str; *ch; ch += utf8_codepoint_len(ch)) {
		struct ui_cell *cell = ui_buffer_at(line, col + ch - str);
		if (!cell)
			return;
		utf8_copy(cell->ch, ch);
		cell->attr = attr;
	}

	if (line > ui_buffer.last_changed_line)
		ui_buffer.last_changed_line = line;
}

void ui_clear(void)
{
	for (int line = 0; line < ui_buffer.lines; ++line)
		for (int col = 0; col < ui_buffer.cols; ++col)
			*ui_buffer_at(line, col) = UI_CELL_DEFAULT;
}

static void update_attributes(struct ui_cell cur, struct ui_cell last)
{
	if (cur.attr.bold != last.attr.bold ||
	    cur.attr.italics != last.attr.italics ||
	    cur.attr.underline != last.attr.underline ||
	    cur.attr.blink != last.attr.blink ||
	    cur.attr.reverse != last.attr.reverse) {
		unibi_print_str(unibi, unibi_exit_attribute_mode, NULL);
		set_fg_rgb(cur.attr.fg);
		set_bg_rgb(cur.attr.bg);
	} else {
		if (!color_equal(cur.attr.fg, last.attr.fg))
			set_fg_rgb(cur.attr.fg);

		if (!color_equal(cur.attr.bg, last.attr.bg))
			set_bg_rgb(cur.attr.bg);
	}

	if (cur.attr.bold)
		unibi_print_str(unibi, unibi_enter_bold_mode, NULL);

	if (cur.attr.italics)
		unibi_print_str(unibi, unibi_enter_italics_mode, NULL);

	if (cur.attr.underline)
		unibi_print_str(unibi, unibi_enter_underline_mode, NULL);

	if (cur.attr.blink)
		unibi_print_str(unibi, unibi_enter_blink_mode, NULL);

	if (cur.attr.reverse)
		unibi_print_str(unibi, unibi_enter_reverse_mode, NULL);
}

void ui_flush(void)
{
	unibi_print_str(unibi, unibi_cursor_home, NULL);

	struct ui_cell last = UI_CELL_DEFAULT;
	for (int line = 0; line < ui_buffer.lines; ++line) {
		for (int col = 0; col < ui_buffer.cols; ++col) {
			struct ui_cell cur = *ui_buffer_at(line, col);
			update_attributes(cur, last);
			printf("%.4s", cur.ch);
			last = cur;
		}

		if (line == ui_buffer.last_changed_line) {
			ui_buffer.last_changed_line = 0;
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
