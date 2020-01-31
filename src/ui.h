#ifndef UI
#define UI

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "torch.h"

void ui_init(void);
void ui_quit(void);

void ui_dimensions(int *lines, int *cols);

typedef uint8_t utf8[4];

struct ui_cell_attr {
	struct color fg, bg;
	bool bold, italic, underline, blink, reverse_video, strikethrough;
};

void ui_draw_at(int line, int col, utf8 ch, struct ui_cell_attr attr);
void ui_draw_str_at(int line, int col, utf8 *str, size_t len, struct ui_cell_attr attr);

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
