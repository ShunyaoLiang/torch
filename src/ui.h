#ifndef TORCH_UI
#define TORCH_UI

#include <stdint.h>
#include <stdbool.h>

void ui_init(void);
void ui_quit(void);
void ui_dimensions(int *lines, int *cols);

struct ui_cell {
	char codepoint[5];
	struct {
		uint8_t r, g, b;
	} fg, bg;
#if 0
	unsigned int bold          : 1,
	             underline     : 1,
	             italic        : 1,
	             reverse_video : 1,
	             blink         : 1,
	             strikethrough : 1;
#endif
};

void ui_draw_at(int line, int col, struct ui_cell cell);
void ui_flush(void);

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

#endif
