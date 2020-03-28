#include "torch.h"
#include "ui.h"

#include <math.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

struct draw_info {
	int view_lines, view_cols;
};

void draw_thing(struct tile *tile, int x, int y, void *context)
{
	struct draw_info *info = context;
	int line = y - clamp(player.posy - info->view_lines / 2, 0, MAP_LINES - info->view_lines);
	int col = x - clamp(player.posx - info->view_cols / 2, 0, MAP_COLS - info->view_cols);
	const char *token = " ";
	struct color color = {0};
	if (tile->entity) {
		token = tile->entity->token;
		color = color_add(color_multiply_by(tile->entity->color, tile->light), tile->lighting);
	} else if (!list_empty(&tile->items)) {
		struct item *item = (struct item *)tile->items.next;
		token = item->token;
		color = color_add(color_multiply_by(item->color, tile->light), tile->lighting);
	} else {
		token = tile->token;
		color = color_add(color_multiply_by(tile->color, tile->light), tile->lighting);
	}
	ui_draw_at(line, col, token, (struct ui_cell_attr) {
		.fg = color,
	});	

	tile->seen_as.color = color;
	tile->seen_as.token = token;
	/* If it is a wall tile and we have seen it in a better light... */
	if (!strcmp(tile->token, "#") && tile->light > tile->seen_as.light) {
		/* ... remember the color of the wall. */
		tile->seen_as.color = color;
		tile->seen_as.light = tile->light;
	}
}

void draw_game(void)
{
	int view_lines, view_cols;
	ui_dimensions(&view_lines, &view_cols);

	for (int y = 0; y < MAP_LINES; ++y)
		for (int x = 0; x < MAP_COLS; ++x) {
			struct tile tile = floor_map_at(cur_floor, y, x);
			if (!tile.seen)
				continue;
			int line = y - clamp(player.posy - view_lines / 2, 0, MAP_LINES - view_lines);
			int col = x - clamp(player.posx - view_cols / 2, 0, MAP_COLS - view_cols);

			ui_draw_at(line, col, tile.seen_as.token, (struct ui_cell_attr) {
				.fg = color_as_grayscale(tile.seen_as.color),
			});
		}

	raycast_at(cur_floor, player.posx, player.posy, max(view_lines, view_cols) / 2, &draw_thing, &(struct draw_info) {
		view_lines, view_cols
	});

	ui_draw_format_at(0, 0, "Fuel: %d Torches: %d HP: %d Floor: %ld",
			  (struct ui_cell_attr){ .fg = { .g = 0xaa } },
			  player_fuel, player_torches, player.combat.hp, cur_floor - floors);
}

void draw_init(void)
{
	extern void torch_flicker(int signal);

	timer_t timer_id;
	timer_create(CLOCK_REALTIME, NULL, &timer_id);
	timer_settime(timer_id, 0, &(struct itimerspec) {
		.it_interval = { 0, 100000000 },
		.it_value = { 0, 100000000 },
	}, NULL);
	signal(SIGALRM, &torch_flicker);
}

struct color color_add(struct color c, struct color a)
{
	return (struct color){
		min(c.r + a.r, 255),
		min(c.g + a.g, 255),
		min(c.b + a.b, 255),
	};
}

struct color color_multiply_by(struct color c, float m)
{
	return (struct color){
		min(c.r * m, 255),
		min(c.g * m, 255),
		min(c.b * m, 255),
	};
}

struct color color_as_grayscale(struct color c)
{
	return (struct color){
		(c.r + c.g + c.b) / 3.f,
		(c.r + c.g + c.b) / 3.f,
		(c.r + c.g + c.b) / 3.f,
	};
}

int color_approximate_256(struct color c)
{
	/* L. */
	return rand() % 256;
}

bool color_equal(struct color a, struct color b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}
