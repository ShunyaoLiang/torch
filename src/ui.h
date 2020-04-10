#ifndef UI
#define UI

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "torch.h"

void ui_init(void);
void ui_quit(void);

void ui_dimensions(int *rows, int *cols);

struct ui_cell_attr {
	struct color fg, bg;
	bool bold, italics, underline, blink, reverse;
};

void ui_draw_at(int row, int col, const char *str, struct ui_cell_attr attr);

/* This has to be a macro because va_lists don't play well with interrupts, and this is used
   by a signal handler. */
#define ui_draw_format_at(row, col, fmt, attr, ...) do { \
	char buf[snprintf(NULL, 0, fmt, __VA_ARGS__) + 1]; \
	sprintf(buf, fmt, __VA_ARGS__); \
	ui_draw_at(row, col, buf, attr); \
} while (0);

void ui_draw_at_incremental(int row, int col, const char *str, struct ui_cell_attr attr, int wait);
void ui_set_attr_at(int row, int col, struct ui_cell_attr attr);

void ui_clear(void);
void ui_flush(void);

enum ui_event_type {
	ui_event_key,
};

struct ui_event {
	enum ui_event_type type;
	union {
		long key; /* ui_event_key */
	};
};

extern bool ui_polling;

struct ui_event ui_poll_event(void);

#endif
