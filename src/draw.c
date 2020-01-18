#include "torch.h"
#include "ui.h"

#include <string.h>
#include <stdlib.h>

#if 0
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
				ui_draw_at(line, col, (struct ui_cell) {
					.codepoint = { [0] = tile.token },
						.fg = {
							.r = r, .g = g, .b = b,
					},
					.bg = {
						.r = 0, .g = 0, .b = 0,
					},
				});
			}
			else {
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

	struct entity * pos;
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
		}
		else {
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
#endif

struct draw_info {
	int view_lines, view_cols;
};

void draw_thing(struct tile *tile, int y, int x, void *context)
{
	struct draw_info * info = context;
	int line = y - clamp(player.posy - info->view_lines / 2, 0, MAP_ROWS - info->view_lines);
	int col = x - clamp(player.posx - info->view_cols / 2, 0, MAP_COLS - info->view_cols);
	if (tile->entity) {
		ui_draw_at(line, col, (struct ui_cell) {
			.codepoint = { [0] = tile->entity->token },
				/*  = tile.entity->color * tile.light + tile.dcolor */
				.fg = color_add(color_multiply_by(tile->entity->color, tile->light), tile->dcolor),
				.bg = { 0, 0, 0 },
		});
	}
	else {
		ui_draw_at(line, col, (struct ui_cell) {
			.codepoint = { [0] = tile->token },
				/*  = tile.color * tile.light + tile.dcolor */
				.fg = color_add(color_multiply_by(tile->color, tile->light), tile->dcolor),
				.bg = { 0, 0, 0 },
		});
	}
}

void draw_shit(void)
{
	int view_lines, view_cols;
	ui_dimensions(&view_lines, &view_cols);

	raycast_at(
		&(struct raycast_params) {
		.callback = &draw_thing,
		.context = &(struct draw_info) {
			.view_lines = view_lines,
			.view_cols = view_cols
		},
		.floor = cur_floor,
		.y = player.posy,
		.x = player.posx,
		.radius = max(view_lines, view_cols) / 2
	});

	size_t needed = snprintf(NULL, 0, "%3d %3d", player_fuel, player_torches) + 1;
	char * buf = malloc(needed);
	sprintf(buf, "%3d %3d", player_fuel, player_torches);
	ui_draw_str_at(1, 0, buf, (struct ui_cell) { .fg = { 0xe2, 0x58, 0x22 } });

	//	ui_draw_at(2, 0, (struct ui_cell){ .codepoint = { [0] = player_fuel }, .fg = { 77, 26, 128 }, });
	//	ui_draw_at(2, 2, (struct ui_cell){ .codepoint = { [0] = player_torches }, .fg = { 77, 26, 128 }, });
}
