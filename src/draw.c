#include "torch.h"

#include <tickit.h>
#include <string.h>

static void intern_pen_set_colour(TickitRenderBuffer *rb, TickitPen *pen, uint8_t r, uint8_t g, uint8_t b);

static int visibility[MAP_LINES][MAP_COLS] = { 0 };

static def_raycast_fn(set_visible)
{
	visibility[y][x] = 1;
}

static void __draw_map(TickitRenderBuffer *rb, TickitPen *pen, tile_map *map)
{
	const int viewy = clamp(player.posy - VIEW_LINES / 2, 0, MAP_LINES - VIEW_LINES);
	const int viewx = clamp(player.posx - VIEW_COLS / 2, 0, MAP_COLS - VIEW_COLS);

	raycast_at(cur_floor->map, player.posy, player.posx, 10, &set_visible, NULL);

	for (int line = 0; line < VIEW_LINES; ++line) {
		for (int col = 0; col < VIEW_COLS; ++col) {
			struct tile tile = floor_map_at(cur_floor, viewy + line, viewx + col);
			uint8_t r = min(tile.r * tile.light + tile.dr, 255);
			uint8_t g = min(tile.g * tile.light + tile.dg, 255);
			uint8_t b = min(tile.b * tile.light + tile.db, 255);

			if (visibility[viewy + line][viewx + col]) {
				intern_pen_set_colour(rb, pen, r, g, b);
				tickit_renderbuffer_char_at(rb, line, col, tile.token);
			} else {
				intern_pen_set_colour(rb, pen, 0, 0, 0);
				tickit_renderbuffer_char_at(rb, line, col, ' ');
			}
		}
	}

	memset(visibility, 0, (sizeof(visibility[MAP_LINES][MAP_COLS]) * MAP_LINES * MAP_COLS));
}

void draw_map(TickitRenderBuffer *rb, TickitPen *pen)
{
	__draw_map(rb, pen, &cur_floor->map);
}

static void __draw_entities(TickitRenderBuffer *rb, TickitPen *pen, entity_list *entities)
{
	raycast_at(cur_floor->map, player.posy, player.posx, 100, &set_visible, NULL);

	struct entity *pos;
	list_for_each_entry(pos, entities, list) {
		int line = pos->posy - clamp((player.posy - VIEW_LINES / 2), 0, MAP_LINES - VIEW_LINES);
		int col = pos->posx - clamp((player.posx - VIEW_COLS / 2), 0, MAP_COLS - VIEW_COLS);
		if (line >= VIEW_LINES || col >= VIEW_COLS)
			continue;
		struct tile tile = floor_map_at(cur_floor, pos->posy, pos->posx);
		uint8_t r = min(pos->r * tile.light + tile.dr, 255);
		uint8_t g = min(pos->g * tile.light + tile.dg, 255);
		uint8_t b = min(pos->b * tile.light + tile.db, 255);

		if (visibility[pos->posy][pos->posx]) {
			intern_pen_set_colour(rb, pen, r, g, b);
			tickit_renderbuffer_char_at(rb, line, col, pos->token);
		} else {
			intern_pen_set_colour(rb, pen, 0, 0, 0);
			tickit_renderbuffer_char_at(rb, line, col, ' ');
		}
	}

	memset(visibility, 0, (sizeof(visibility[MAP_LINES][MAP_COLS]) * MAP_LINES * MAP_COLS));
}

void draw_entities(TickitRenderBuffer *rb, TickitPen *pen)
{
	__draw_entities(rb, pen, &cur_floor->entities);
}

static void intern_pen_set_colour(TickitRenderBuffer *rb, TickitPen *pen, uint8_t r, uint8_t g, uint8_t b)
{
	tickit_pen_set_colour_attr(pen, TICKIT_PEN_FG, 1);
	TickitPenRGB8 rgb = { .r = r, .g = g, .b = b };
	tickit_pen_set_colour_attr_rgb8(pen, TICKIT_PEN_FG, rgb);
	tickit_renderbuffer_setpen(rb, pen);
}
