#include "floor.h"
#include "entity.h"
#include "list.h"
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
		.colour = { .r = 17, .g = 70, .b = 13 },
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

struct blend_data {
	TickitPenRGB8 colour;
	float light;
} blend_buffer[MAP_LENGTH][MAP_WIDTH];

void player_update(struct entity *this)
{
	blend_buffer[this->posy][this->posx].light = 1.f;
}

void light_update(struct entity *this)
{
	blend_buffer[this->posy-1][this->posx-1].colour.r = 255;
	blend_buffer[this->posy-1][this->posx].colour.r = 255;
	blend_buffer[this->posy-1][this->posx+1].colour.r = 255;
	blend_buffer[this->posy][this->posx-1].colour.r = 255;
	blend_buffer[this->posy][this->posx].colour.r = 255;
	blend_buffer[this->posy][this->posx+1].colour.r = 255;
	blend_buffer[this->posy+1][this->posx-1].colour.r = 255;
	blend_buffer[this->posy+1][this->posx].colour.r = 255;
	blend_buffer[this->posy+1][this->posx+1].colour.r = 255;

	blend_buffer[this->posy-1][this->posx-1].light = 0.5f;
	blend_buffer[this->posy-1][this->posx].light = 0.5f;
	blend_buffer[this->posy-1][this->posx+1].light = 0.5f;
	blend_buffer[this->posy][this->posx-1].light = 0.5f;
	blend_buffer[this->posy][this->posx].light = 1.f;
	blend_buffer[this->posy][this->posx+1].light = 0.5f;
	blend_buffer[this->posy+1][this->posx-1].light = 0.5f;
	blend_buffer[this->posy+1][this->posx].light = 0.5f;
	blend_buffer[this->posy+1][this->posx+1].light = 0.5f;
}

struct blend_data blend_buffer_at(int y, int x)
{
	if (y >= 0 && y < MAP_LENGTH && x >= 0 && x < MAP_WIDTH) {
		return blend_buffer[y][x];
	} else {
		return (struct blend_data){ 0 };
	}
}

void blend_buffer_flush_light(void)
{
	for (int y = 0; y < MAP_LENGTH; ++y) {
		for (int x = 0; x < MAP_WIDTH; ++x) {
			cur_floor->map[y][x].light = blend_buffer[y][x].light;
		}
	}
}

struct tile floor_map_at(const struct floor *floor, int y, int x)
{
	if (y >= 0 && y < MAP_LENGTH && x >= 0 && x < MAP_WIDTH) {
		return floor->map[y][x];
	} else {
		return (struct tile){ .sprite = { .token = ' ' } };
	}
}

struct sprite blend_sprite(struct sprite sprite, struct blend_data blend)
{
	sprite.colour.r += blend.colour.r;
	sprite.colour.g += blend.colour.g;
	sprite.colour.b += blend.colour.b;

	sprite.colour.r *= blend.light;
	sprite.colour.g *= blend.light;
	sprite.colour.b *= blend.light;

	return sprite;
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
			to_draw = blend_sprite(to_draw, blend_data);

			tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 3);
			tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, to_draw.colour);
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

		const struct blend_data blend_data = blend_buffer_at(pos->posy, pos->posx);
		struct sprite to_draw = pos->sprite;
		to_draw = blend_sprite(to_draw, blend_data);

		tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 3);
		tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, to_draw.colour);
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

static int main_win_key(TickitWindow *win, TickitEventFlags flags, void *info, void *user)
{
	/* ... */

	/* Clear the blend buffer. */
	memset(blend_buffer, 0, sizeof(blend_buffer));

	update_entities();

	/* Write light levels to the actual tile map. */
	blend_buffer_flush_light();

	tickit_window_expose(win, NULL);
	return 1;
}
