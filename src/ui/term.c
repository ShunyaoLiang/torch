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

#define CSI "\x1b["

#define MIN(a, b) (a < b ? a : b)

typedef uint8_t utf8[4];
static void utf8_copy(utf8 dest, utf8 src);

struct ui_cell {
	utf8 ch;
	struct ui_cell_attr attr;
};

#define UI_CELL_DEFAULT (struct ui_cell) { \
	.ch = " ", \
};

struct ui_buffer {
	size_t lines;
	size_t cols;
	struct ui_cell **buffer;
};

static void ui_buffer_realloc(struct ui_buffer *b);
static struct ui_cell *ui_buffer_at(int line, int col);
static bool ui_buffer_in_bounds(int line, int col);

static struct ui_buffer ui_buffer = {0};

static unibi_term *unibi = NULL;
struct unibi_ext {
	size_t set_fg_rgb;
	size_t set_bg_rgb;
};
static struct unibi_ext unibi_ext;

static void unibi_print_str(unibi_term *unibi, enum unibi_string s, unibi_var_t param[9]);
static void unibi_print_ext_str(unibi_term *unibi, size_t i, unibi_var_t param[9]);
static void unibi_print_fmt(const char *fmt, unibi_var_t param[9]);

static void term_resize_handler(int signal);

struct term_dimensions {
	size_t lines, cols;
};
static struct term_dimensions term_dimensions(void);

static void generate_sgr(char *sgr, struct ui_cell cur, struct ui_cell last);

static TermKey *termkey = NULL;

void ui_init(void)
{
	ui_buffer_realloc(&ui_buffer);
	/* Realloc the buffer on resize. */
	signal(SIGWINCH, &term_resize_handler);

	termkey = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8);

	const char *term = getenv("TERM");
	unibi = unibi_from_term(term);
	unibi_ext.set_fg_rgb = unibi_add_ext_str(unibi, "ext_set_fg_rgb",
		CSI"38;2;%p1%d;%p2%d;%p3%dm");
	unibi_ext.set_bg_rgb = unibi_add_ext_str(unibi, "ext_set_bg_rgb",
		CSI"48;2;%p1%d;%p2%d;%p3%dm");

	unibi_print_str(unibi, unibi_enter_ca_mode, NULL);
	unibi_print_str(unibi, unibi_cursor_invisible, NULL);
	unibi_print_str(unibi, unibi_cursor_home, NULL);
	unibi_var_t zero = unibi_var_from_num(0);
	unibi_print_ext_str(unibi, unibi_ext.set_fg_rgb, (unibi_var_t[9]){zero, zero, zero});
	unibi_print_ext_str(unibi, unibi_ext.set_bg_rgb, (unibi_var_t[9]){zero, zero, zero});
}

static void ui_buffer_realloc(struct ui_buffer *b)
{
	/* Get new terminal dimensions. */
	struct term_dimensions new_dimensions = term_dimensions();	

	/* Resize top array of pointers. */
	b->buffer = realloc(b->buffer, new_dimensions.lines * sizeof(struct ui_cell *));

	/* Resize existing lines. */
	for (int line = 0; line < MIN(b->lines, new_dimensions.lines); ++line) {
		b->buffer[line] = realloc(b->buffer[line], new_dimensions.cols * sizeof(struct ui_cell));
	}

	/* Allocate new lines. */
	for (int line = b->lines; line < new_dimensions.lines; ++line) {
		b->buffer[line] = calloc(new_dimensions.cols, sizeof(struct ui_cell));
	}

	/* Update buffer dimensions. */
	b->lines = new_dimensions.lines;
	b->cols = new_dimensions.cols;
}

static struct term_dimensions term_dimensions(void)
{
	struct winsize winsize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsize);
	return (struct term_dimensions) {
		.lines = winsize.ws_row,
		.cols = winsize.ws_col,
	};
}

static void term_resize_handler(int signal)
{
	ui_buffer_realloc(&ui_buffer);
	(void)signal;
}

static void unibi_print_str(unibi_term *unibi, enum unibi_string s, unibi_var_t param[9])
{
	const char *fmt = unibi_get_str(unibi, s);
	unibi_print_fmt(fmt, param);
}

static void unibi_print_ext_str(unibi_term *unibi, size_t i, unibi_var_t param[9])
{
	const char *fmt = unibi_get_ext_str(unibi, i);
	unibi_print_fmt(fmt, param);
}

static void unibi_print_fmt(const char *fmt, unibi_var_t param[9])
{
	size_t needed = unibi_run(fmt, param, NULL, 0) + 1;
	char *buf = malloc(needed);
	buf[needed - 1] = '\0';
	unibi_run(fmt, param, buf, needed);
	fputs(buf, stdout);
}

void ui_quit(void)
{
	puts(CSI "?25h" CSI "?1049l");

	termkey_destroy(termkey);
	unibi_destroy(unibi);
}

void ui_dimensions(int *lines, int *cols)
{
	if (lines)
		*lines = ui_buffer.lines;
	if (cols)
		*cols = ui_buffer.cols;
}

void ui_draw_at(int line, int col, utf8 ch, struct ui_cell_attr attr)
{
	if (!ui_buffer_in_bounds(line, col))
		return;
	struct ui_cell *cell = ui_buffer_at(line, col);
	utf8_copy(cell->ch, ch);
	cell->attr = attr;
}

static struct ui_cell *ui_buffer_at(int line, int col)
{
	if (ui_buffer_in_bounds(line, col))
		return &ui_buffer.buffer[line][col];
	else
		return NULL;
}

static bool ui_buffer_in_bounds(int line, int col)
{
	return line >= 0 && line < ui_buffer.lines && col >= 0 && col < ui_buffer.cols;
}

static void utf8_copy(utf8 dest, utf8 src)
{
	memcpy(dest, src, sizeof(utf8));
}

void ui_draw_str_at(int line, int col, utf8 *str, size_t len, struct ui_cell_attr attr)
{
	for (size_t i = 0; i < len; ++i)
		ui_draw_at(line, col, str[i], attr);
}

void ui_clear(void)
{
	for (int line = 0; line < ui_buffer.lines; ++line)
		for (int col = 0; col < ui_buffer.cols; ++col)
			*ui_buffer_at(line, col) = UI_CELL_DEFAULT;
}

void ui_flush(void)
{
	struct ui_cell last = UI_CELL_DEFAULT;
	char sgr[55]; /* "Look Mum, it's an SGR string" -Q */

	unibi_print_str(unibi, unibi_cursor_home, NULL);
	for (int line = 0; line < ui_buffer.lines; ++line) {
		for (int col = 0; col < ui_buffer.cols; ++col) {
			struct ui_cell cur = *ui_buffer_at(line, col);
			generate_sgr(sgr, cur, last);
			printf("%s%.4s", sgr, cur.ch);
			last = cur;
		}
		putchar('\n');
	}
}

static void generate_sgr(char *sgr, struct ui_cell cur, struct ui_cell last)
{
	int params[16];
	size_t nparams = 0;

	/* Refer to the ECMA 48 standard. */
	if (!color_equal(cur.attr.fg, last.attr.fg)) {
		/* 38 = Set foreground color */
		params[nparams++] = 38;
		/* 2 = Use RGB */
		params[nparams++] = 2;
		/* Add each RGB channel. */
		params[nparams++] = cur.attr.fg.r;
		params[nparams++] = cur.attr.fg.g;
		params[nparams++] = cur.attr.fg.b;
	}

	if (!color_equal(cur.attr.bg, last.attr.bg)) {
		/* 48 = Set background color */
		params[nparams++] = 48;
		/* 2 = Use RGB */
		params[nparams++] = 2;
		/* Add each RGB channel. */
		params[nparams++] = cur.attr.bg.r;
		params[nparams++] = cur.attr.bg.g;
		params[nparams++] = cur.attr.bg.b;
	}

	if (cur.attr.bold != last.attr.bold)
		params[nparams++] = cur.attr.bold ? 1 : 22;

	if (cur.attr.italic != last.attr.italic)
		params[nparams++] = cur.attr.italic ? 3 : 23;

	if (cur.attr.underline != last.attr.underline)
		params[nparams++] = cur.attr.underline ? 4 : 24;

	if (cur.attr.blink != last.attr.blink)
		params[nparams++] = cur.attr.blink ? 5 : 25;

	if (cur.attr.reverse_video != last.attr.reverse_video)
		params[nparams++] = cur.attr.reverse_video ? 7 : 27;

	if (cur.attr.strikethrough != last.attr.strikethrough)
		params[nparams++] = cur.attr.strikethrough? 9 : 29;

	if (!nparams) {
		*sgr = '\0';
		return;
	}

	/* Add CSI to beginning of SGR string. */
	char *iter = sgr;
	iter += sprintf(iter, CSI);

	/* Write each parameter except the last one. */
	for (int i = 0; i < nparams - 1; ++i)
		iter += sprintf(iter, "%d;", params[i]);
	/* No semicolon on the last parameter. */
	iter += sprintf(iter, "%d", params[nparams - 1]);
	/* End the sequence. */
	sprintf(iter, "m");
}

bool ui_polling = false;

struct ui_event ui_poll_event(void)
{
	TermKeyKey key;
	struct ui_event e;

	ui_polling = true;
	while (ui_polling)
		switch (termkey_waitkey(termkey, &key)) {
		case TERMKEY_RES_KEY:
			e.key = key.code.codepoint;
			ui_polling = false;
		default:
			break;
		}

	return e;
}

#ifdef TEST

int main(void)
{
	ui_init();

	ui_clear();
	ui_draw_at(4, 4, (utf8) {'@', '\0'}, (struct ui_cell_attr) {
		.fg = {0xe2, 0x58, 0x22}
	});
	ui_flush();

	getchar();

	ui_quit();	
}

#endif
