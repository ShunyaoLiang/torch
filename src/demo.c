#include "torch.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct floor *cur_floor = &floors[0];

struct entity demo_new_snake(int y, int x);
void demo_place_snake(int y, int x);

void demo_floor_load_map(const char *filename)
{
//	FILE *mapfp = fopen(filename, "r");
	floor_map_generate(&floors[0], CAVE);

	/* Make this not shit please. */
	for (size_t line = 0; line < MAP_LINES; ++line) {
		for (size_t col = 0; col < MAP_COLS; ++col) {
//			fscanf(mapfp, "%c", &demo_floor.map[line][col].token);
			floors[0].map[line][col].light = 0;
			floors[0].map[line][col].color = (struct color){ 51, 51, 51 };

#if 0
			if (demo_floor.map[line][col].token != '.') {
				demo_floor.map[line][col].g = 51;
				demo_floor.map[line][col].b = 51;
			}
#endif
		}
//		(void)fgetc(mapfp);
	}

//	fclose(mapfp);

	for (int i = 0; i < 20; ++i) {
		int y = rand() % MAP_LINES;
		int x = rand() % MAP_COLS;
		demo_place_snake(y, x);
	}
}

int drawn_to[MAP_LINES][MAP_COLS] = { 0 };

struct light_info {
	tile_map *map;
	float bright;
	int y, x;
	struct color color;
};

#include <assert.h>

void cast_light_at(struct tile *tile, int y, int x, void *context)
{
	if (drawn_to[y][x])
		return;
	struct light_info *info = context;
	drawn_to[y][x] = 1;
	int distance = sqrt(pow(y - info->y, 2) + pow(x - info->x, 2));
	const float dlight = info->bright / (distance + 1) / (distance + 1);
	if (y == info->y && x == info->x) {
		(*info->map)[y][x].light += info->bright;
	} else {
		(*info->map)[y][x].light += dlight;
//		assert((*info->map)[y][x].light > 0);
		/*                        = info->r * dlight + tile.dcolor */
		(*info->map)[y][x].dcolor = color_add(color_multiply_by(info->color, dlight), tile->dcolor);
	}
}

def_entity_fn(demo_player_update)
{
	float bright = player_lantern_on && player_fuel > 0 ? 0.5f : 0.1f;
	if (player_lantern_on) {
		if (player_fuel > 0) {
			player_fuel--;
		} else {
			player_lantern_on = false;
		}
	}

	//cast_light(this->floor->map, y, x, 6, 0.3f, this->r, this->g, this->b);

	raycast_at(*(struct raycast_params) {
		.callback = &cast_light_at,
		.context = &(struct light_info) {
		.map = &this->floor->map,
			.bright = bright, .y = y, .x = x,
			.color = this->color,
		}
		.floor = this->floor->map,
		.y = this->posy,
		.x = this->posx,
		.radius = sqrt(1.f / (1.f / 255)),
	});

	memset(drawn_to, 0, (sizeof(drawn_to[0][0]) * MAP_LINES * MAP_COLS));
}

def_entity_fn(demo_torch_update)
{
	int y = (rand() % 3 - 1);
	int x = (rand() % 3 - 1);

//	entity_move_pos_rel(this, y, x);

	y = this->posy;
	x = this->posx;
	//cast_light(this->floor->map, y, x, 6, 1.f, this->r, this->g, this->b);
	int radius = sqrt(1.f / (1.f / 255));
	raycast_at(this->floor->map, y, x, radius, &cast_light_at,
		&(struct light_info) {
			.map = &this->floor->map,
			.bright = 1.f, .y = y, .x = x, 
			.color = this->color,
		});
	memset(drawn_to, 0, (sizeof(drawn_to[0][0]) * MAP_LINES * MAP_COLS));
}

def_entity_fn(demo_torch_destroy)
{

}

void demo_add_entities(void)
{
#if 0
	struct entity torch = {
		.r = 0xe2, .g = /*0x58*/0, .b = 0x22,
		.token = 't',
		.posy = 12, .posx = 7,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
	};

	struct entity *t1 = malloc(sizeof(torch));
	struct entity *t2 = malloc(sizeof(torch));

	memcpy(t1, &torch, sizeof(torch));
	torch.r = 0x5e;
	torch.g = /*0xba*/0;
	torch.b = 0xc9;
	torch.posy = 12;
	torch.posx = 8;
	memcpy(t2, &torch, sizeof(torch));

	floor_add_entity(cur_floor, t1);
	floor_add_entity(cur_floor, t2);

	struct entity rock = {
		.r = 170, .g = 170, .b = 170,
		.token = '?',
		.posy = 13, 13,
		.update = NULL,
		.destroy = NULL,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
	};

	struct entity *r = malloc(sizeof(rock));
	memcpy(r, &rock, sizeof(rock));
	floor_add_entity(cur_floor, r);
#endif
}

struct entity demo_new_torch(int y, int x)
{
	struct entity torch = {
		.color = {
			.r = 0xe2, .g = 0x58, .b = 0x22,
		},
		.token = '!',
		.posy = y, .posx = x,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
		.blocks_light = 0,
	};

	return torch;
}

def_input_key_fn(place_torch)
{
	if (!player_torches)
		return 0;
	player_torches--;
	int y = player.posy;
	int x = player.posx;
	struct ui_event event = ui_poll_event();
	switch (event.key) {
	case 'h': x--; break;
	case 'j': y++; break;
	case 'k': y--; break;
	case 'l': x++; break;
	}
	struct entity torch = demo_new_torch(y, x);
	struct entity *t = malloc(sizeof(torch));
	memcpy(t, &torch, sizeof(torch));
	return floor_add_entity(cur_floor, t);
}

def_input_key_fn(demo_get_fuel)
{
	player_fuel += 10;
	return 0;
}

void demo_place_snake(int y, int x)
{
	struct entity snake = demo_new_snake(y, x);
	struct entity *s = malloc(sizeof(snake));
	memcpy(s, &snake, sizeof(snake));
	floor_add_entity(&floors[0], s);
}

def_entity_fn(demo_snake_update)
{
	int y = 0;
	int x = 0;
	if (this->posy < player.posy)
		y = 1;
	else if (this->posy > player.posy)
		y = -1;
	if (this->posx < player.posx)
		x = 1;
	else if (this->posx > player.posx)
		x = -1;

	entity_move_pos_rel(this, y, x);

	if (floor_map_at(this->floor, this->posy, this->posx).light > 0.2)
		entity_move_pos_rel(this, -y, -x);
	if (abs(this->posy - player.posy) == 1 && abs(this->posx - player.posx) == 1) {
		ui_clear();
		ui_flush();
		ui_quit();
		puts("lol you fucking died nerd");
		exit(0);
	}
}

struct entity demo_new_snake(int y, int x)
{
	struct entity snake = {
		.color = {
			.r = 0xa, .g = 0xCA, .b = 0xa,
		},
		.token = 's',
		.posy = y, .posx = x,
		.update = demo_snake_update,
		.destroy = NULL,
		.list = LIST_HEAD_INIT(snake.list),
		.floor = &floors[0],
		.blocks_light = 0,
	};

	return snake;
}

#if 0
void raycast_octant_at(tile_map map, int y, int x, int radius, int row,
	float start_slope, float end_slope, int octant, raycast_fn *callback,
	void *context);

void raycast_at(tile_map map, int y, int x, int radius, raycast_fn *callback,
	void *context)
{
	callback(y, x, context);
	for (int octant = 0; octant < 8; ++octant)
		raycast_octant_at(map, y, x, radius, 1, 1.0, 0.0, octant,
			callback, context);
}

static void raycast_octant_at(tile_map map, int y, int x, int radius, int row,
	float start_slope, float end_slope, int octant, raycast_fn *callback,
	void *context)
{
	static int transforms[4][8] = {
		{1, 0, 0, -1, -1, 0, 0, 1},
		{0, 1, -1, 0, 0, -1, 1, 0},
		{0, 1, 1, 0, 0, -1, -1, 0},	
		{1, 0, 0, 1, -1, 0, 0, -1},
	};

	if (start_slope < end_slope) {
		return;
	}

	float next_start_slope = start_slope;
	for (; row <= radius; ++row) {
		bool blocked = false;
		for (int dx = -row, dy = -row; dx <= 0; dx++) {
			float bl_slope = (dx - 0.5) / (dy + 0.5);
			float tr_slope = (dx + 0.5) / (dy - 0.5);
			if (start_slope < tr_slope) {
				continue;
			} else if (end_slope > bl_slope) {
				break;
			}

			int xx = transforms[0][octant];
			int xy = transforms[1][octant];
			int yx = transforms[2][octant];
			int yy = transforms[3][octant];
			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;
			if ((sax < 0 && abs(sax) > x) ||
				(say < 0 && abs(say) > y)) {
				continue;
			}

			int ax = x + sax;
			int ay = y + say;
			if (!floor_map_in_bounds(ay, ax)) {
				continue;
			}

			uint radius2 = radius * radius;
			if ((uint)(dx * dx + dy * dy) < radius2) {
				callback(ay, ax, context);
			}

			struct tile tile = map[ay][ax];
			if (blocked) {
				if (!tile.walk || (tile.entity ? tile.entity->blocks_light : tile.entity)) {
					next_start_slope = tr_slope;
					continue;
				} else {
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (!tile.walk || (tile.entity ? tile.entity->blocks_light : tile.entity)) {
				blocked = true;
				next_start_slope = tr_slope;
				raycast_octant_at(map, y, x, radius, row + 1,
					start_slope, bl_slope, octant,
					callback, context);
			}
		}
		if (blocked) {
			break;
		}
	}
}
#endif

struct floor floors[] = {
	[0] = {0},
};
