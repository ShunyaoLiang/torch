#include "ui.h"
#include "termkey.h"
#include "torch.h"

#include <sys/ioctl.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CSI "\e["
#define ALTERNATE_SCREEN "?1049h"
#define NORMAL_SCREEN "?1049l"
#define HIDE_CURSOR "?25l"
#define SHOW_CURSOR "?25h"

static void get_terminal_size();
static void handle_resize(int signal);

static void ui_buffer_realloc(void);
static void __ui_buffer_realloc(int lines, int cols);
static bool ui_buffer_in_bounds(int line, int col);

static void move_cursor(int line, int col);

int ui_lines;
int ui_cols;

static struct ui_cell sentinel_cell;

struct ui_cell **ui_buffer;

TermKey *termkey_instance;

void ui_init(void)
{
	get_terminal_size();
	signal(SIGWINCH, &handle_resize);

	ui_buffer_realloc();

	printf(CSI ALTERNATE_SCREEN CSI "38;2;0;0;0;48;2;0;0;0;22;23;24;25;27;29m" CSI HIDE_CURSOR CSI "H" CSI "J");

	termkey_instance = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8);
}

static void get_terminal_size()
{
	struct winsize sz;
	ioctl(0, TIOCGWINSZ, &sz);
	ui_lines = sz.ws_row;
	ui_cols = sz.ws_col;
}

static void handle_resize(int signal)
{
	get_terminal_size();
	ui_buffer_realloc();
}

static void ui_buffer_realloc(void)
{
	__ui_buffer_realloc(ui_lines, ui_cols);
}

static void __ui_buffer_realloc(int lines, int cols)
{
	/* Keep track of the previous buffer size. */
	static int old_lines = 0;

	/* Initial allocation. Should only be ran once. */
	if (!old_lines) {
		ui_buffer = malloc(sizeof(struct ui_cell *) * lines);
		if (!ui_buffer)
			goto fail;

		for (int line = 0; line < lines; ++line) {
			ui_buffer[line] = malloc(sizeof(struct ui_cell) * cols);
			if (!ui_buffer[line])
				goto fail;
		}
	} else {
		/* Free all uneeded lines. */
		for (int line = old_lines - 1; line >= ui_lines; --line) {
			free(ui_buffer[line]);
		}

		/* Resize the top array of pointers. */
		struct ui_cell **tmp = realloc(ui_buffer, sizeof(struct ui_cell *) * lines);
		if (!tmp)
			goto fail;
		ui_buffer = tmp;

		/* Resize existing lines. */
		for (int line = 0; line < min(old_lines, lines); ++line) {
			struct ui_cell *tmp = realloc(ui_buffer[line], sizeof(struct ui_cell) * cols);
			if (!tmp)
				goto fail;
			ui_buffer[line] = tmp;
		}

		/* Allocate all new lines. */
		for (int line = old_lines; line < lines; ++line) {
			ui_buffer[line] = malloc(sizeof(struct ui_cell) * cols);
			if (!ui_buffer[line])
				goto fail;
		}
	}

	old_lines = lines;

	return;

fail:
	free(ui_buffer);
	perror(__func__);
	abort();
}

void ui_quit(void)
{
	termkey_destroy(termkey_instance);

	printf(CSI SHOW_CURSOR CSI NORMAL_SCREEN);
}

void ui_dimensions(int *lines, int *cols)
{
	if (lines)
		*lines = ui_lines;
	if (cols)
		*cols = ui_cols;
}

void ui_draw_at(int line, int col, struct ui_cell cell)
{
	if (ui_buffer_in_bounds(line, col)) {
		ui_buffer[line][col] = cell;
	}
}

static bool ui_buffer_in_bounds(int line, int col)
{
	return line >= 0 && line < ui_lines && col >= 0 && col < ui_cols;
}

void ui_clear(void)
{
	for (int line = 0; line < ui_lines; ++line) {
		memset(ui_buffer[line], 0, sizeof(struct ui_cell) * ui_cols);
		for (int col = 0; col < ui_cols; ++col) {
			ui_buffer[line][col].codepoint[0] = ' ';
			ui_buffer[line][col].codepoint[1] = '\0';
		}
	}
}

void ui_flush(void)
{
	static char sgr_buf[100]; // look mum it's a SGR string
	struct ui_cell this, last = sentinel_cell;

	for (int line = 0; line < ui_lines; ++line) {
		move_cursor(line, 1);
		for (int col = 0; col < ui_cols; ++col) {
			this = ui_buffer[line][col];

			int nparams = 0, sgr_params[16];
			int sgrlen = 0;

			if (this.fg.r != last.fg.r || this.fg.g != last.fg.g || this.fg.b != last.fg.b) {
				sgr_params[nparams++] = 38;
				sgrlen += 3;
				sgr_params[nparams++] = 2;
				sgrlen += 2;
				sgr_params[nparams++] = this.fg.r;
				sgrlen += snprintf(NULL, 0, "%d;", this.fg.r);
				sgr_params[nparams++] = this.fg.g;
				sgrlen += snprintf(NULL, 0, "%d;", this.fg.g);
				sgr_params[nparams++] = this.fg.b;
				sgrlen += snprintf(NULL, 0, "%d;", this.fg.b);
			}

			if (this.bg.r != last.bg.r || this.bg.g != last.bg.g || this.bg.b != last.bg.b) {
				sgr_params[nparams++] = 48;
				sgrlen += 3;
				sgr_params[nparams++] = 2;
				sgrlen += 2;
				sgr_params[nparams++] = this.bg.r;
				sgrlen += snprintf(NULL, 0, "%d;", this.bg.r);
				sgr_params[nparams++] = this.bg.g;
				sgrlen += snprintf(NULL, 0, "%d;", this.bg.g);
				sgr_params[nparams++] = this.bg.b;
				sgrlen += snprintf(NULL, 0, "%d;", this.bg.b);
			}

			if (this.bold != last.bold) {
				sgr_params[nparams++] = this.bold ? 1 : 22;
				sgrlen += this.bold ? 1 : 2;
			}

			if (this.italic != last.italic) {
				sgr_params[nparams++] = this.italic ? 3 : 23;
				sgrlen += this.italic ? 1 : 2;
			}

			if (this.under_ != last.under_) {
				sgr_params[nparams++] = this.under_ ? 4 : 24;
				sgrlen += this.under_ ? 1 : 2;
			}

			if (this.blink != last.blink) {
				sgr_params[nparams++] = this.blink ? 5 : 25;
				sgrlen += this.blink ? 1 : 2;
			}

			if (this.reverse_vid != last.reverse_vid) {
				sgr_params[nparams++] = this.reverse_vid ? 7 : 27;
				sgrlen += this.reverse_vid ? 1 : 2;
			}

			if (this.strikethru != last.strikethru) {
				sgr_params[nparams++] = this.strikethru? 9 : 29;
				sgrlen += this.strikethru ? 1 : 2;
			}

			if (sgrlen) {
				char *iter = sgr_buf;

				iter += sprintf(iter, "\e[");
				for (int i = 0; i < nparams-1; i++)
					iter += sprintf(iter, "%d;", sgr_params[i]);
				if (nparams > 0) // no semicolon for last parameter
					iter += sprintf(iter, "%d", sgr_params[nparams-1]);
				sprintf(iter, "m");
			} else
				sgr_buf[0] = '\0';

			printf("%s%s", sgr_buf, this.codepoint);

			last = this;
		}
	}
	sentinel_cell = this;
}

static void move_cursor(int line, int col)
{
	if (ui_buffer_in_bounds(line, col))
		if (line == 1 && col == 1)
			printf(CSI "H");
		else if (col == 1)
			printf(CSI "%dH", line);
		else
			printf(CSI "%d;%dH", line, col);
	else
		fprintf(stderr, "move_cursor: %d,%d are not in bounds [0:%d,%d)", line, col, ui_lines, ui_cols);
}

void ui_draw_str_at(int line, int col, const char *str, struct ui_cell attr)
{
	while (*str) {
		attr.codepoint[0] = *str++;
		ui_draw_at(line, col++, attr);
	}
}

struct ui_event ui_poll_event(void)
{
	bool cont =  true;
	TermKeyKey key;
	struct ui_event event;
	
	while (cont) switch (termkey_waitkey(termkey_instance, &key)) {
	case TERMKEY_RES_KEY:
		switch (key.type) {
		case TERMKEY_TYPE_UNICODE:
			event.key = key.code.codepoint;
			cont = false;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return event;
}

#ifdef TEST

#include <assert.h>

int main(void)
{
	ui_init();
	assert(ui_buffer);

	int lines;
	ui_dimensions(&lines, NULL);

	ui_clear();

	ui_draw_at(lines - 1, 0, (struct ui_cell){
		.codepoint = "L",
		.fg = {
			.r = 85, .g = 43, .b = 200,
		},
	});
	assert(strcmp(ui_buffer[lines - 1][0].codepoint, "L") == 0);

	ui_draw_str_at(3, 3, "Test", (struct ui_cell){.fg = {19, 103, 76}});

	ui_flush();
	struct ui_event event = ui_poll_event();

	ui_quit();
}

#endif
