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

void xy_to_linecol(int x, int y, int *restrict line, int *restrict col)
{
	int view_lines, view_cols;
	ui_dimensions(&view_lines, &view_cols);

	if (view_lines > MAP_LINES)	
		*line = y - player.posy + view_lines / 2;
	else
		*line = y - clamp(player.posy - view_lines / 2, 0, MAP_LINES - view_lines);

	if (view_cols > MAP_COLS)	
		*col = x - player.posx + view_cols / 2;
	else
		*col = x - clamp(player.posx - view_cols / 2, 0, MAP_COLS - view_cols);
}

void draw_thing(struct tile *tile, int x, int y, void *context)
{
	int line, col;
	xy_to_linecol(x, y, &line, &col);
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
	if (tile->light > 0)
		ui_draw_at(line, col, token, (struct ui_cell_attr) {
			.fg = color,
		});	

	/* Unless it is a wall tile and we haven't seen it in a better light... */
	if (strcmp(tile->token, "#") || tile->light > tile->seen_as.light) {
		/* ... remember the color of the wall. */
		tile->seen_as.color = color;
		tile->seen_as.light = tile->light;
		/* Don't remember the token if it's the player's token. */
		if (strcmp(token, "@"))
			tile->seen_as.token = token;
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
			int line, col;
			xy_to_linecol(x, y, &line, &col);

			/* If it is not a floor tile... */
			if (strcmp(tile.seen_as.token, ".")) {
				ui_draw_at(line, col, tile.seen_as.token, (struct ui_cell_attr) {
					.fg = color_multiply_by(color_as_grayscale(tile.seen_as.color), 0.6),
				});
			} else {
				/* Draw floor tiles at a constant color, so they're visible. */
				ui_draw_at(line, col, tile.seen_as.token, (struct ui_cell_attr) {
					.fg = (struct color) {0x7, 0x7, 0x7},
				});
			}
		}

	raycast_at(cur_floor, player.posx, player.posy, max(view_lines-1, view_cols) / 2, &draw_thing, &(struct draw_info) {
		view_lines-1, view_cols
	});

	struct color barfg = { 0xff, 0x50, 0 };
	struct color hpbg = { 100, 20, 20 };

	ui_draw_format_at(view_lines-1, 0, "Fuel: %-3d ", (struct ui_cell_attr){ .fg = barfg }, player_fuel);

	ui_draw_format_at(view_lines-1, 10, "Torches: %-4d ", (struct ui_cell_attr){ .fg = barfg }, player_torches);

#define HPBARLEN 10
	ui_draw_format_at(view_lines-1, 24, "HP: %-5d ", (struct ui_cell_attr){ .fg = barfg }, player.combat.hp);
	int shaded_len = roundf((float)HPBARLEN * player.combat.hp / player.combat.hp_max);
	for(int col = 24; col < 24 + shaded_len; col++)
		ui_set_attr_at(view_lines-1, col, (struct ui_cell_attr){ .fg = barfg, .bg = hpbg });

	ui_draw_format_at(view_lines-1, 34, "Floor: %-ld", (struct ui_cell_attr){ .fg = barfg }, cur_floor - floors);

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
