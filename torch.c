#include <tickit.h>
#include <stdlib.h>
#include <stdio.h>

struct entity {
	char token;
	int xpos, ypos;

	void (*update)(struct entity *kore);
};

void player_update(struct entity *kore);

struct player {
	struct entity e;
} you = {{'@', 5, 5, &player_update}};

void player_update(struct entity *kore)
{
	struct player *this = (struct player *)kore;
}

void player_left(void)
{
	you.e.xpos--;
}

void player_down(void)
{
	you.e.ypos++;
}

void player_up(void)
{
	you.e.ypos--;
}

void player_right(void)
{
	you.e.xpos++;
}

void (*key_handlers[])(void) = {
	['h'] = player_left,
	['j'] = player_down,
	['k'] = player_up,
	['l'] = player_right,
};

int event_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitKeyEventInfo *key_info = info;
	
	if (key_handlers[key_info->str[0]]) {
		key_handlers[key_info->str[0]]();
	}

	tickit_window_expose(win, NULL);
}

char map[24][80] = { 0 };

int render(TickitWindow *win, TickitEventFlags flag, void *info, void *data)
{
	TickitExposeEventInfo *expose_info = info;
	TickitRenderBuffer *rb = expose_info->rb;

	tickit_renderbuffer_eraserect(rb, &expose_info->rect);

	for (int y = 0; y < 24; ++y) {
		for (int x = 0; x < 80; ++x) {
			const char text[] = { map[y][x], '\0' };
			tickit_renderbuffer_text_at(rb, y, x, text);
		}
	}

	tickit_renderbuffer_text_at(rb, you.e.ypos, you.e.xpos, "@");
}

int main(void)
{
	Tickit *t = tickit_new_stdio();
	TickitWindow *rootwin = tickit_get_rootwin(t);

	tickit_window_take_focus(rootwin);
	tickit_window_set_cursor_visible(rootwin, false);

	tickit_window_bind_event(rootwin, TICKIT_WINDOW_ON_KEY, 0, &event_key, NULL);
	tickit_window_bind_event(rootwin, TICKIT_WINDOW_ON_EXPOSE, 0, &render, NULL);

	FILE *mapfile = fopen("map", "r");
	for (int y = 0; y < 24; ++y) {
		for (int x = 0; x < 80; ++x) {
			map[y][x] = fgetc(mapfile);
		}

		(void)fgetc;
	}
	fclose(mapfile);

	tickit_run(t);
	
	tickit_unref(t);
	return EXIT_SUCCESS;
}
