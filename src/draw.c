#include "torch.h"
#include "ui.h"

#include <string.h>

static int visibility[MAP_LINES][MAP_COLS] = { 0 };

static def_raycast_fn(set_visible)
{
	visibility[y][x] = 1;
}

void draw_map(void)
{
	int view_lines, view_cols;
	ui_dimensions(&view_lines, &view_cols);

	const int viewy = clamp(player.posy - view_lines / 2, 0, MAP_LINES - view_lines);
	const int viewx = clamp(player.posx - view_cols / 2, 0, MAP_COLS - view_cols);

	raycast_at(cur_floor->map, player.posy, player.posx, 100, &set_visible, NULL);

	for (int line = 0; line < view_lines; ++line) {
		for (int col = 0; col < view_cols; ++col) {
			struct tile tile = floor_map_at(cur_floor, viewy + line, viewx + col);
			uint8_t r = min(tile.r * tile.light + tile.dr, 255);
			uint8_t g = min(tile.g * tile.light + tile.dg, 255);
			uint8_t b = min(tile.b * tile.light + tile.db, 255);

			if (visibility[viewy + line][viewx + col]) {
				ui_draw_at(line, col, (struct ui_cell){
					.codepoint = { [0] = tile.token },
					.fg = {
						.r = r, .g = g, .b = b,
					},
					.bg = {
						.r = 0, .g = 0, .b = 0,
					},
				});
			} else {
				ui_draw_at(line, col, (struct ui_cell) {
					.codepoint = " ",
					.fg = {
						.r = 0, .g = 0, .b = 0,
					},
					.bg = {
						.r = 0, .g = 0, .b = 0,
					},
				});
			}
		}
	}

	memset(visibility, 0, (sizeof(visibility[MAP_LINES][MAP_COLS]) * MAP_LINES * MAP_COLS));
}

void draw_entities(void)
{
	int view_lines, view_cols;
	ui_dimensions(&view_lines, &view_cols);

	raycast_at(cur_floor->map, player.posy, player.posx, 100, &set_visible, NULL);

	struct entity *pos;
	list_for_each_entry(pos, &cur_floor->entities, list) {
		int line = pos->posy - clamp((player.posy - view_lines / 2), 0, MAP_LINES - view_lines);
		int col = pos->posx - clamp((player.posx - view_cols / 2), 0, MAP_COLS - view_cols);
		if (line >= view_lines || col >= view_cols)
			continue;
		struct tile tile = floor_map_at(cur_floor, pos->posy, pos->posx);
		uint8_t r = min(pos->r * tile.light + tile.dr, 255);
		uint8_t g = min(pos->g * tile.light + tile.dg, 255);
		uint8_t b = min(pos->b * tile.light + tile.db, 255);

		if (visibility[pos->posy][pos->posx]) {
			ui_draw_at(line, col, (struct ui_cell) {
				.codepoint = { [0] = pos->token },
				.fg = {
					.r = r, .g = g, .b = b,
				},
				.bg = {
					.r = 0, .g = 0, .b = 0,
				},
			});
		} else {
			ui_draw_at(line, col, (struct ui_cell) {
				.codepoint = " ",
				.fg = {
					.r = 0, .g = 0, .b = 0,
				},
				.bg = {
					.r = 0, .g = 0, .b = 0,
				},
			});
		}
	}

	memset(visibility, 0, (sizeof(visibility[MAP_LINES][MAP_COLS]) * MAP_LINES * MAP_COLS));
}
