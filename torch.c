#include "torch.h"

#include <tickit.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define def_main_win_key_fn(name) void name(void)
typedef def_main_win_key_fn(main_win_key_fn);

TickitWindowEventFn main_win_on_key;
TickitWindowEventFn main_win_draw;

void update_entities(entity_list *entities);

static void demo_floor_load_map(const char *filename);
static void demo_add_entities(void);

void draw_map(TickitRenderBuffer *rb, TickitPen *pen);
void draw_entities(TickitRenderBuffer *rb, TickitPen *pen);
void whatdoicallthis(TickitRenderBuffer *rb, TickitPen *pen, uint8_t r, uint8_t g, uint8_t b);

struct floor demo_floor;
struct floor *cur_floor = &demo_floor;

struct entity player = {
	.r = 255, .g = 102, .b = 77,
	.token = '@',
	.posy = 0, .posx = 0,
	.update = NULL,
	.destroy = NULL,
	.list = LIST_HEAD_INIT(player.list),
};

int main(void)
{
	srand(time(NULL));

	demo_floor_load_map("map");
	INIT_LIST_HEAD(&demo_floor.entities);
	list_add(&player.list, &cur_floor->entities);

	demo_add_entities();

	Tickit *tickit_instance = tickit_new_stdio();
	
	TickitWindow *main_win = tickit_get_rootwin(tickit_instance);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_KEY, 0, main_win_on_key, NULL);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_EXPOSE, 0, main_win_draw, NULL);

	tickit_run(tickit_instance);

	tickit_window_close(main_win);

	tickit_unref(tickit_instance);

	return EXIT_SUCCESS;
}

static void demo_floor_load_map(const char *filename)
{
	FILE *mapfd = fopen(filename, "r");

	/* Make this not shit please. */
	for (size_t line = 0; line < MAP_LINES; ++line) {
		for (size_t col = 0; col < MAP_COLS; ++col) {
			fscanf(mapfd, "%1s", &demo_floor.tile_map[line][col].token);
			demo_floor.tile_map[line][col].light = 0;
			demo_floor.tile_map[line][col].r = 255;
			demo_floor.tile_map[line][col].g = 255;
			demo_floor.tile_map[line][col].b = 255;
		}
	}

	fclose(mapfd);
}

def_entity_fn(demo_torch_update)
{
	this->posx += (rand() % 3 - 1);
	this->posy += (rand() % 3 - 1);
}

def_entity_fn(demo_torch_destroy)
{

}

static void demo_add_entities(void)
{
	struct entity torch = {
		.r = 0xe2, .g = 0x58, .b = 0x22,
		.token = 't',
		.posy = 3, .posx = 3,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list)
	};

	struct entity *t1 = malloc(sizeof(torch));
	struct entity *t2 = malloc(sizeof(torch));

	memcpy(t1, &torch, sizeof(torch));
	torch.r = 0x5e;
	torch.g = 0xba;
	torch.b = 0xc9;
	torch.posy = 7;
	torch.posx = 5;
	memcpy(t2, &torch, sizeof(torch));

	list_add(&t1->list, &cur_floor->entities);
	list_add(&t2->list, &cur_floor->entities);
}

/* Temporary workaround. Allows (most) keyboard input to be mapped to an
   initialized NULL value. */
def_main_win_key_fn(nothing)
{
	return;
}

def_main_win_key_fn(player_move_left);
def_main_win_key_fn(player_move_down);
def_main_win_key_fn(player_move_up);
def_main_win_key_fn(player_move_right);

main_win_key_fn *main_win_keymap[] = {
	['h'] = player_move_left,
	['j'] = player_move_down,
	['k'] = player_move_up,
	['l'] = player_move_right,
	[255] = nothing,
};

int main_win_on_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitKeyEventInfo *key = info;
	switch (key->type) {
	case TICKIT_KEYEV_TEXT:
		if (main_win_keymap[*key->str])
			main_win_keymap[*key->str]();
	}

	update_entities(&cur_floor->entities);

	tickit_window_expose(win, NULL);
	return 1;
}

int main_win_draw(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitExposeEventInfo *exposed = info;
	TickitRenderBuffer *rb = exposed->rb;
	tickit_renderbuffer_eraserect(rb, &exposed->rect);

	TickitPen *pen = tickit_pen_new();
	tickit_pen_set_colour_attr(pen, TICKIT_PEN_BG, 0);
	tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_BG, (TickitPenRGB8){0, 0, 0});
	tickit_renderbuffer_setpen(rb, pen);

	draw_map(rb, pen);
	draw_entities(rb, pen);

	return 1;
}

void update_entities(entity_list *entities)
{
	struct entity *pos;
	list_for_each_entry(pos, entities, list) {
		if (pos->update)
			pos->update(pos);
	}
}

struct tile tile_map_at(struct tile (*map)[MAP_LINES][MAP_COLS], int y, int x)
{
	if (y >= 0 && y < MAP_LINES && x >= 0 && x < MAP_COLS) {
		return (*map)[y][x];
	} else {
		return (struct tile){ 0 };
	}
}

void __draw_map(TickitRenderBuffer *rb, TickitPen *pen, struct tile (*map)[MAP_LINES][MAP_COLS])
{
	const int viewy = player.posy - VIEW_LINES / 2;
	const int viewx = player.posx - VIEW_COLS / 2;

	for (int line = 0; line < VIEW_LINES; ++line) {
		for (int col = 0; col < VIEW_COLS; ++col) {
			struct tile tile = tile_map_at(map, viewy + line, viewx + col);
			whatdoicallthis(rb, pen, tile.r, tile.g, tile.b);
			tickit_renderbuffer_text_at(rb, line, col, (char[]){tile.token, '\0'});
		}
	}
}

void draw_map(TickitRenderBuffer *rb, TickitPen *pen)
{
	__draw_map(rb, pen, &cur_floor->tile_map);
}

void __draw_entities(TickitRenderBuffer *rb, TickitPen *pen, entity_list *entities)
{
	struct entity *pos;
	list_for_each_entry(pos, entities, list) {
		int line = pos->posy - (player.posy - VIEW_LINES / 2);
		int col = pos->posx - (player.posx - VIEW_COLS / 2);
		whatdoicallthis(rb, pen, pos->r, pos->g, pos->b);
		tickit_renderbuffer_text_at(rb, line, col, (char[]){ pos->token, '\0'});
	}
}

void draw_entities(TickitRenderBuffer *rb, TickitPen *pen)
{
	__draw_entities(rb, pen, &cur_floor->entities);
}

void whatdoicallthis(TickitRenderBuffer *rb, TickitPen *pen, uint8_t r, uint8_t g, uint8_t b)
{
	tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 1);
	TickitPenRGB8 rgb = { .r = r, .g = g, .b = b };
	tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, rgb);
	tickit_renderbuffer_setpen(rb, pen);
}

def_main_win_key_fn(player_move_left)
{
	player.posx--;
}

def_main_win_key_fn(player_move_down)
{
	player.posy++;
}

def_main_win_key_fn(player_move_up)
{
	player.posy--;
}

def_main_win_key_fn(player_move_right)
{
	player.posx++;
}

#if 0

#include "floor.h"
#include "entity.h"
#include "list.h"
#include "blend.h"
#include "torch.h"

#include <tickit.h>
#include <stdio.h>
#include <string.h>

struct floor demo;
struct floor *cur_floor = &demo;

void player_update(struct entity *this);
void light_update(struct entity *this);

struct entity player = {
	.posx = 0,
	.posy = 0,
	.sprite = { 
		.colour = { .r = 85, .g = 178, .b = 194 },
		.token = '@',
	},
	.update = player_update,
	.speed = 1,
	.order = 10,
	.list = LIST_HEAD_INIT(player.list),
};

static void demo_load_map(const char *filename);

static int main_win_draw(TickitWindow *win, TickitEventFlags flags, void *info, void *user);
static int main_win_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user);

int main(void)
{
	INIT_LIST_HEAD(&cur_floor->entities);
	list_add(&player.list, &cur_floor->entities);

	struct light torch = {
		.e = {
			.posx = 2,
			.posy = 2,
			.sprite = { 
				.colour = { .r = 255 },
				.token = 't',
			},
			.update = light_update,
			.speed = 1,
			.order = 0,
			.list = LIST_HEAD_INIT(torch.e.list),
		},
		.emit = 0.2f,
		.duration = 10,
	};
	list_add(&torch.e.list, &cur_floor->entities);

	demo_load_map("map");

	Tickit *tickit_instance = tickit_new_stdio();

	TickitWindow *main_win = tickit_get_rootwin(tickit_instance);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_EXPOSE, 0, main_win_draw, NULL);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_KEY, 0, main_win_key, NULL);

	tickit_run(tickit_instance);
	
	tickit_window_close(main_win);

	tickit_unref(tickit_instance);

	return EXIT_SUCCESS;
}

static void demo_load_map(const char *filename)
{
	FILE *mapfd = fopen(filename, "r");

	/* Make this not shit please. */
	for (size_t line = 0; line < MAP_LENGTH; ++line) {
		for (size_t col = 0; col < MAP_WIDTH; ++col) {
			fscanf(mapfd, "%1s", &demo.map[line][col].sprite.token);
		}
	}

	fclose(mapfd);
}


void player_update(struct entity *this)
{
	blend_buffer_add(this->posy, this->posx, this->sprite.colour.r,
		this->sprite.colour.g, this->sprite.colour.b, 0.0f);
}

void light_update(struct entity *this)
{
	blend_buffer_add(this->posy, this->posx, 255, 0, 0, 1.f);
	blend_buffer_add(this->posy, this->posx+1, 255, 0, 0, 1.f);
	blend_buffer_add(this->posy, this->posx+2, 255, 0, 0, 1.f / 4);
	blend_buffer_add(this->posy, this->posx+3, 255, 0, 0, 1.f / 9);
	blend_buffer_add(this->posy, this->posx+4, 255, 0, 0, 1.f / 16);
	blend_buffer_add(this->posy, this->posx+5, 255, 0, 0, 1.f / 25);

	blend_buffer_add(this->posy+1, this->posx, 255, 0, 0, 1.f);
	blend_buffer_add(this->posy+2, this->posx, 255, 0, 0, 1.f / 4);
	blend_buffer_add(this->posy+3, this->posx, 255, 0, 0, 1.f / 9);
	blend_buffer_add(this->posy+4, this->posx, 255, 0, 0, 1.f / 16);
	blend_buffer_add(this->posy+5, this->posx, 255, 0, 0, 1.f / 25);

	blend_buffer_add(this->posy+2, this->posx, 0, 0, 255, 1.f / 25);
	blend_buffer_add(this->posy+3, this->posx, 0, 0, 255, 1.f / 16);
	blend_buffer_add(this->posy+4, this->posx, 0, 0, 255, 1.f / 9);
	blend_buffer_add(this->posy+5, this->posx, 0, 0, 255, 1.f / 4);
	blend_buffer_add(this->posy+6, this->posx, 0, 0, 255, 1.f);

}

struct tile floor_map_at(const struct floor *floor, int y, int x)
{
	if (y >= 0 && y < MAP_LENGTH && x >= 0 && x < MAP_WIDTH) {
		return floor->map[y][x];
	} else {
		return (struct tile){ .sprite = { .token = ' ' } };
	}
}

void draw_map(TickitRenderBuffer *rb, TickitPen *pen, int viewy, int viewx)
{
	for (int line = 0; line < VIEW_LINES; ++line) {
		for (int col = 0; col < VIEW_COLS; ++col) {
			/* Get map coordinates of the tile to be drawn at (col, line). */
			const int drawy = viewy + line;
			const int drawx = viewx + col;

			const struct blend_data blend_data = blend_buffer_at(drawy, drawx);
			struct sprite to_draw = floor_map_at(cur_floor, drawy, drawx).sprite;
			TickitPenRGB8 colour = blend_sprite(blend_data, to_draw);

			tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 3);
			tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, colour);
			tickit_renderbuffer_setpen(rb, pen);
			tickit_renderbuffer_text_at(rb, line, col, (char[]){ to_draw.token, '\0' });
		}
	}
}

void draw_entities(TickitRenderBuffer *rb, TickitPen *pen, int viewy, int viewx)
{
	struct entity *pos = NULL;
	list_for_each_entry(pos, &cur_floor->entities, list) {
		/* Get terminal coordinates of the entity to be drawn. */
		const int line = pos->posy - viewy;
		const int col = pos->posx - viewx;

		/* Don't draw the entity if they are beyond the viewport. */
		if (line < 0 || line >= VIEW_LINES || col < 0 || col >= VIEW_COLS)
			continue;

		const struct blend_data blend_data = blend_buffer_at(pos->posy, pos->posx);
		struct sprite to_draw = pos->sprite;
		TickitPenRGB8 colour = blend_sprite(blend_data, to_draw);

		tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 3);
		tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, colour);
		tickit_renderbuffer_setpen(rb, pen);
		tickit_renderbuffer_text_at(rb, line, col, (char[]){ to_draw.token, '\0' });
	}
}

static int main_win_draw(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitExposeEventInfo *exposed = info;
	TickitRenderBuffer *rb = exposed->rb;
	tickit_renderbuffer_eraserect(rb, &exposed->rect);
	TickitPen *pen = tickit_pen_new();
	tickit_pen_set_colour_attr(pen, TICKIT_PEN_BG, 0);
	tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_BG, (TickitPenRGB8){ .r = 0, .g = 0, .b = 0});

	/* Get map coordinates of the top-left tile to be drawn. */
	const int viewy = player.posy - VIEW_LINES / 2;
	const int viewx = player.posx - VIEW_COLS / 2;

	draw_map(rb, pen, viewy, viewx);
	draw_entities(rb, pen, viewy, viewx);

	return 1;
}

void update_entities(void)
{
	struct entity *pos = NULL;
	list_for_each_entry(pos, &cur_floor->entities, list) {
		pos->update ? pos->update(pos) : fprintf(stderr, "torch: entity lacks update function");
	}
}

static void player_move_left(void);
static void player_move_down(void);
static void player_move_up(void);
static void player_move_right(void);

void (*main_win_keymap[])(void) = {
	['h'] = player_move_left,
	['j'] = player_move_down,
	['k'] = player_move_up,
	['l'] = player_move_right,
};

static int main_win_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	TickitKeyEventInfo *key = info;
	switch (key->type) {
	case TICKIT_KEYEV_TEXT: /* Pain. */
		main_win_keymap[*key->str] ? main_win_keymap[*key->str]() : 0;
	}

	blend_buffer_clear();
	update_entities();
	/* Write light levels to the actual tile map. */
	blend_buffer_flush_light();

	tickit_window_expose(win, NULL);
	return 1;
}

static void player_move_left(void)
{
	player.posx--;
}

static void player_move_down(void)
{
	player.posy++;
}

static void player_move_up(void)
{
	player.posy--;
}

static void player_move_right(void)
{
	player.posx++;
}

#endif
