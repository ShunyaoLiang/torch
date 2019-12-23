#include "ui.h"
#include "termkey.h"

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

static void get_terminal_size();
static void handle_resize(int signal);

static void ui_buffer_realloc(void);
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

	printf("\e[38;2;0;0;0;48;2;0;0;0m");
	printf(CSI ALTERNATE_SCREEN);

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
	static int old_lines = 0, old_cols = 0;

	/* Allocate memory the first time. */
	if (!old_lines) {
		ui_buffer = malloc(sizeof(struct ui_cell *) * ui_lines);
	} else if (ui_lines != old_lines) {
		for (int line = old_lines - 1; line >= ui_lines; --line) {
			free(ui_buffer[line]);
		}
		ui_buffer = realloc(ui_buffer, sizeof(struct ui_cell *) * ui_lines);
		if (!ui_buffer)
			goto fail;
		for (int line = old_lines; line < ui_lines; ++line) {
			ui_buffer[line] = malloc(sizeof(struct ui_cell) * ui_cols);
			if (!ui_buffer[line])
				goto fail;
		}
	}

	if (ui_cols != old_cols) {
		for (int line = 0; line < ui_lines; ++line) {
			if (line < old_lines)
				ui_buffer[line] = realloc(ui_buffer[line], sizeof(struct ui_cell) * ui_cols);
			else
				ui_buffer[line] = malloc(sizeof(struct ui_cell) * ui_cols);
			if (!ui_buffer[line])
				goto fail;
		}
	}

	old_lines = ui_lines;
	old_cols = ui_cols;

	return;

fail:
	perror(__func__);
	abort();
}

void ui_quit(void)
{
	termkey_destroy(termkey_instance);

	printf(CSI NORMAL_SCREEN);
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

void ui_flush(void)
{
	static char sgr_buf[100]; // look mum it's a SGR string
	struct ui_cell this, last = sentinel_cell;

	printf("\e[H");

	for (int line = 0; line < ui_lines; ++line) {
		for (int col = 0; col < ui_cols; ++col) {
			this = ui_buffer[line][col];

			int nparams = 0, sgr_params[5]; // rgb foreground only
			int sgrlen = 0;

			if(this.fg.r != last.fg.r || this.fg.g != last.fg.g || this.fg.b != last.fg.b) {
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

			if(this.bg.r != last.bg.r || this.bg.g != last.bg.g || this.bg.b != last.bg.b) {
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

			if(sgrlen) {
				char *iter = sgr_buf;

				iter += sprintf(iter, "\e[");
				for(int i = 0; i < nparams-1; i++)
					iter += sprintf(iter, "%d;", sgr_params[i]);
				if(nparams > 0) // no semicolon for last parameter
					iter += sprintf(iter, "%d", sgr_params[nparams-1]);
				sprintf(iter, "m");
			} else
				sgr_buf[0] = '\0';

			printf("%s%s", sgr_buf, this.codepoint);

			last = this;
		}
		puts("\r");
	}
	sentinel_cell = this;
}

static void move_cursor(int line, int col)
{
	if (ui_buffer_in_bounds(line, col))
		printf(CSI "%d;%dH", line, col);
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
		}
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

	ui_draw_at(lines - 1, 0, (struct ui_cell){
		.codepoint = "L",
		.fg = {
			.r = 85, .g = 43, .b = 200,
		},
	});
	assert(strcmp(ui_buffer[lines - 1][0].codepoint, "L") == 0);

	ui_flush();
	struct ui_event event = ui_poll_event();

	ui_quit();
}

#endif
