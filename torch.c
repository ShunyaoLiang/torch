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
