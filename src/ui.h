#ifndef TORCH_UI
#define TORCH_UI

#include "torch.h"

#include <stdint.h>
#include <stdbool.h>

void ui_init(void);
void ui_quit(void);
void ui_dimensions(int *lines, int *cols);

struct ui_cell {
	char codepoint[5];
	struct color fg, bg;
	unsigned int bold        : 1,
	             italic      : 1,
	             under_      : 1,
	             blink       : 1,
	             reverse_vid : 1,
	             strikethru  : 1;
};

void ui_draw_at(int line, int col, struct ui_cell cell);
void ui_clear(void);
void ui_flush(void);

/* Hacky garbage. */
void ui_draw_str_at(int line, int col, const char *str, struct ui_cell attr);

enum ui_event_type {
	UI_EVENT_KEY,
};

enum ui_event_key {
	UI_EVENT_KEY_a = 'a',
	UI_EVENT_KEY_A = 'A',
};

struct ui_event {
	enum ui_event_type type;	
	union {
		long key;
	};
};

struct ui_event ui_poll_event(void);

extern bool ui_blocking;

#endif
