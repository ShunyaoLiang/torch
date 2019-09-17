#include "torch.h"

#include <tickit.h>
#include <stdlib.h>
#include <stdio.h>

struct dungeon demo = {
	.map = {0},
	.entities = LIST_HEAD_INIT(demo.entities)
};

struct dungeon *current_dungeon = &demo;

#define VIEW_LINES 23
#define VIEW_COLS  79

struct entity player = {
	.posx = 0, .posy = 0
};

static void key_left(char key);
static void key_up(char key);
static void key_down(char key);
static void key_right(char key);

typedef void (*keymap_fn)(char key);
keymap_fn keymap[] = {
	['h'] = key_left,
	['j'] = key_down,
	['k'] = key_up,
	['l'] = key_right
};

static int root_handle_key(TickitWindow *win, TickitEventFlags flags, void *info, void *data)
{
	TickitKeyEventInfo *key = info;
	switch (key->type) {
	case TICKIT_KEYEV_TEXT: /* Pain. */
		if (keymap[*key->str])
			keymap[*key->str](*key->str);
	}

	tickit_window_expose(win, NULL);

	return 1;
}

static int root_handle_expose(TickitWindow *win, TickitEventFlags flags, void *info, void *data)
{
	TickitExposeEventInfo *exposed = info;
	TickitRenderBuffer *rb = exposed->rb;

	tickit_renderbuffer_eraserect(rb, &exposed->rect);

	const int viewy = player.posy - (VIEW_LINES / 2);
	const int viewx = player.posx - (VIEW_COLS / 2);

	/* Draw map. */
	for (int y = 0; y < VIEW_LINES; ++y) {
		for (int x = 0; x < VIEW_COLS; ++x) {
			const int drawy = viewy + y;
			const int drawx = viewx + x;

			if (drawy >= 0 && drawy < MAP_LINES && drawx >= 0 && drawx < MAP_COLS)
				tickit_renderbuffer_text_at(rb, y, x, (char[]){current_dungeon->map[drawy][drawx].sprite, '\0'});
			else
				tickit_renderbuffer_text_at(rb, y, x, " ");
		}
	}

	/* Draw non-player entities. */
	struct entity *pos = NULL;
	list_for_each_entry(pos, &current_dungeon->entities, list) {
		tickit_renderbuffer_text_at(rb, pos->posy - viewy, pos->posx - viewx, (char[]){pos->sprite, '\0'});
	}

	/* Draw the player. */
	tickit_renderbuffer_text_at(rb, VIEW_LINES / 2 + 1, VIEW_COLS / 2 + 1, "@");

	return 1;
}

void load_demo_map(const char *filename)
{
	FILE *mapfd = fopen(filename, "r");

	for (size_t line = 0; line < MAP_LINES; ++line) {
		fscanf(mapfd, "%s", demo.map[line]);
	}

	fclose(mapfd);
}

int main(int argc, char *argv[])
{
	Tickit *tickit_instance = NULL;
	tickit_instance = tickit_new_stdio();

	TickitWindow *root = tickit_get_rootwin(tickit_instance);
	tickit_window_bind_event(root, TICKIT_WINDOW_ON_KEY, 0, &root_handle_key, NULL);
	tickit_window_bind_event(root, TICKIT_WINDOW_ON_EXPOSE, 0, &root_handle_expose, NULL);

	load_demo_map("map");

	/* Don't fucking touch this. */
	INIT_LIST_HEAD(&current_dungeon->entities);

	struct entity snake = {
		.posx = 3,
		.posy = 4,
		.sprite = 's',
		.list = LIST_HEAD_INIT(snake.list)
	};

	struct entity demon = {
		.posx = 6,
		.posy = 6,
		.sprite = 'd',
		.list = LIST_HEAD_INIT(demon.list)
	};

	list_add(&snake.list, &current_dungeon->entities);
	list_add(&demon.list, &current_dungeon->entities);

	struct entity *pos = NULL;
	list_for_each_entry(pos, &current_dungeon->entities, list) {
		
	}

	tickit_run(tickit_instance);

	tickit_window_close(root);

	tickit_unref(tickit_instance);

	return EXIT_SUCCESS;
}

static void key_left(char key)
{
	player.posx--;
}

static void key_up(char key)
{
	player.posy--;
}

static void key_down(char key)
{
	player.posy++;
}

static void key_right(char key)
{
	player.posx++;
}
