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

int ui_lines;
int ui_cols;

struct ui_cell **ui_buffer;

TermKey *termkey_instance;

void ui_init(void)
{
	get_terminal_size();
	signal(SIGWINCH, &handle_resize);

	ui_buffer_realloc();

	termkey_instance = termkey_new(STDIN_FILENO, TERMKEY_FLAG_UTF8);

	printf(CSI ALTERNATE_SCREEN);
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
		ui_buffer = realloc(ui_buffer, sizeof(struct ui_cell *) * ui_lines);
		if (!ui_buffer)
			goto fail;
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
	if (line >= 0 && line < ui_lines && col >= 0 && col < ui_cols) {
		ui_buffer[line][col] = cell;
	}
}

void ui_flush(void)
{
	struct ui_cell last = ui_buffer[0][0];
	for (int line = 0; line < ui_lines; ++line) {
		for (int col = 0; col < ui_cols; ++col) {
			printf("\e[38;2;%03d;%03d;%03d;48;2;%03d;%03d;%03dm%s",
			       ui_buffer[line][col].fg.r,
			       ui_buffer[line][col].fg.g,
			       ui_buffer[line][col].fg.b,
			       ui_buffer[line][col].bg.r,
			       ui_buffer[line][col].bg.g,
			       ui_buffer[line][col].bg.b,
			       ui_buffer[line][col].codepoint);
			last = ui_buffer[line][col];
		}
		if (line != ui_lines - 1)
			putchar('\n');
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
