#include "torch.h"

#include <tickit.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define def_main_win_key_fn(name) void name(void)
typedef def_main_win_key_fn(main_win_key_fn);

def_entity_fn(demo_player_update);

TickitWindowEventFn main_win_on_key;
TickitWindowEventFn main_win_draw;

void update_entities(entity_list *entities);

void entity_move_pos_rel(struct entity *e, int y, int x);

static void demo_floor_load_map(const char *filename);
static void demo_add_entities(void);

void draw_map(TickitRenderBuffer *rb, TickitPen *pen);
void draw_entities(TickitRenderBuffer *rb, TickitPen *pen);
void whatdoicallthis(TickitRenderBuffer *rb, TickitPen *pen, uint8_t r, uint8_t g, uint8_t b);

struct floor demo_floor;
struct floor *cur_floor = &demo_floor;

FILE *debug_log;

struct entity player = {
	.r = 151, .g = 151, .b = 151,
	.token = '@',
	.posy = 17, .posx = 28,
	.update = demo_player_update,
	.destroy = NULL,
	.list = LIST_HEAD_INIT(player.list),
};

int main(void)
{
	srand(time(NULL));
	debug_log = fopen("debug_log", "w");

	demo_floor_load_map("map");
	INIT_LIST_HEAD(&demo_floor.entities);
	floor_add_entity(cur_floor, &player);

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
			fscanf(mapfd, "%c", &demo_floor.map[line][col].token);
			demo_floor.map[line][col].light = 0;
			demo_floor.map[line][col].r = 51;
			demo_floor.map[line][col].g = 51;
			demo_floor.map[line][col].b = 51;

			if (demo_floor.map[line][col].token != '.') {
				demo_floor.map[line][col].g = 150;
				demo_floor.map[line][col].b = 150;
			}
		}
		(void)fgetc(mapfd);
	}

	fclose(mapfd);
}

int floor_in_bounds(int line, int col)
{
	return line >= 0 && line < MAP_LINES && col >= 0 && col < MAP_COLS;
}

#include <math.h>

def_entity_fn(demo_player_update)
{
	int y = this->posy;
	int x = this->posx;
	struct tile (*map)[MAP_LINES][MAP_COLS] = &cur_floor->map;
	const int radius = 4;
	for (int drawy = y - radius; drawy <= y + radius; ++drawy) {
		for (int drawx = x - radius; drawx <= x + radius; ++drawx) {	
			const int distance = sqrt((drawx - x) * (drawx - x) + (drawy - y) * (drawy - y)); 
			if (!(distance <= radius) || !floor_in_bounds(drawy, drawx))
				continue;

			const float dlight = 0.2f / distance;
			(*map)[drawy][drawx].light += dlight;
			(*map)[drawy][drawx].dr += this->r * dlight;
			(*map)[drawy][drawx].dg += this->g * dlight;
			(*map)[drawy][drawx].db += this->b * dlight;
		}
	}
}

def_entity_fn(demo_torch_update)
{
	int y = (rand() % 3 - 1);
	int x = (rand() % 3 - 1);

	entity_move_pos_rel(this, y, x);

	y = this->posy;
	x = this->posx;
	struct tile (*map)[MAP_LINES][MAP_COLS] = &cur_floor->map;
	const int radius = 8;
	for (int drawy = y - radius; drawy <= y + radius; ++drawy) {
		for (int drawx = x - radius; drawx <= x + radius; ++drawx) {	
			const int distance = floor(sqrt((drawx - x) * (drawx - x) + (drawy - y) * (drawy - y))); 
			if (!(distance <= radius) || !floor_in_bounds(drawy, drawx))
				continue;

			const float dlight = 0.7f / distance / distance;
			(*map)[drawy][drawx].light += dlight;
			(*map)[drawy][drawx].dr += this->r * dlight;
			(*map)[drawy][drawx].dg += this->g * dlight;
			(*map)[drawy][drawx].db += this->b * dlight;
		}
	}
}

def_entity_fn(demo_torch_destroy)
{

}

static void demo_add_entities(void)
{
	struct entity torch = {
		.r = 0xe2, .g = 0x58, .b = 0x22,
		.token = 't',
		.posy = 12, .posx = 7,
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
	torch.posy = 10;
	torch.posx = 14;
	memcpy(t2, &torch, sizeof(torch));

	floor_add_entity(cur_floor, t1);
	floor_add_entity(cur_floor, t2);
}

void clear_lights(void)
{
	for (size_t line = 0; line < MAP_LINES; ++line) {
		for (size_t col = 0; col < MAP_COLS; ++col) {
			cur_floor->map[line][col].light = 0;
			cur_floor->map[line][col].dr = 0;
			cur_floor->map[line][col].dg = 0;
			cur_floor->map[line][col].db = 0;
		}
	}
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

void entity_move_pos(struct entity *e, int y, int x)
{
	if (floor_map_in_bounds(y, x) && !cur_floor->map[y][x].on) {
		struct tile *tile = &cur_floor->map[e->posy][e->posx];
		tile->on = NULL;
		cur_floor->map[y][x].on = e;
		e->posy = y;
		e->posx = x;
	}
}

void entity_move_pos_rel(struct entity *e, int y, int x)
{
	entity_move_pos(e, y + e->posy, x + e->posx);
}

void __draw_map(TickitRenderBuffer *rb, TickitPen *pen, struct tile (*map)[MAP_LINES][MAP_COLS])
{
	const int viewy = player.posy - VIEW_LINES / 2;
	const int viewx = player.posx - VIEW_COLS / 2;

	for (int line = 0; line < VIEW_LINES; ++line) {
		for (int col = 0; col < VIEW_COLS; ++col) {
			struct tile tile = floor_map_at(map, viewy + line, viewx + col);
			whatdoicallthis(rb, pen, tile.r * tile.light + tile.dr, tile.g * tile.light + tile.dg, tile.b * tile.light + tile.db);
			tickit_renderbuffer_text_at(rb, line, col, (char[]){tile.token, '\0'});
		}
	}
}

void draw_map(TickitRenderBuffer *rb, TickitPen *pen)
{
	__draw_map(rb, pen, &cur_floor->map);
}

void __draw_entities(TickitRenderBuffer *rb, TickitPen *pen, entity_list *entities)
{
	struct entity *pos;
	list_for_each_entry(pos, entities, list) {
		int line = pos->posy - (player.posy - VIEW_LINES / 2);
		int col = pos->posx - (player.posx - VIEW_COLS / 2);
		if (line >= VIEW_LINES || col >= VIEW_COLS)
			continue;
		struct tile tile = floor_map_at(&cur_floor->map, pos->posy, pos->posx);
		fprintf(debug_log, "dr: %d dg: %d db: %d", tile.dr, tile.dg, tile.db);
		whatdoicallthis(rb, pen, pos->r + tile.dr, pos->g + tile.dg, pos->b + tile.db);
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
	entity_move_pos_rel(&player, 0, -1);
}

def_main_win_key_fn(player_move_down)
{
	entity_move_pos_rel(&player, 1, 0);
}

def_main_win_key_fn(player_move_up)
{
	entity_move_pos_rel(&player, -1, 0);
}

def_main_win_key_fn(player_move_right)
{
	entity_move_pos_rel(&player, 0, 1);
}
