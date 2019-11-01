#include "torch.h"

#include <tickit.h>

main_win_key_fn *main_win_keymap[] = {
	['h'] = player_move_left,
	['j'] = player_move_down,
	['k'] = player_move_up,
	['l'] = player_move_right,
	[255] = NULL,
};

int main_win_on_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitKeyEventInfo *key = info;
	switch (key->type) {
	case TICKIT_KEYEV_TEXT:
		if (main_win_keymap[*key->str])
			main_win_keymap[*key->str]();
	}

	floor_map_clear_lights();
	floor_update_entities(cur_floor);

	tickit_window_expose(win, NULL);
	return 1;
}

int main_win_draw(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitExposeEventInfo *exposed = info;
	TickitRenderBuffer *rb = exposed->rb;
//	tickit_renderbuffer_eraserect(rb, &exposed->rect);

	TickitPen *pen = tickit_pen_new();
	tickit_pen_set_colour_attr(pen, TICKIT_PEN_BG, 0);
	tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_BG, (TickitPenRGB8){0, 0, 0});
	tickit_renderbuffer_setpen(rb, pen);

	draw_map(rb, pen);
	draw_entities(rb, pen);

	return 1;
}



