#include "torch.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

struct floor demo_floor;
struct floor *cur_floor = &demo_floor;

/* Deprecated */
static void cast_light(tile_map map, int y, int x, int radius, float bright,
	int r, int g, int b);
static void cast_octant(tile_map map, int y, int x, int radius, float bright,
	int r, int g, int b, int row, float start_slope, float end_slope,
	int xx, int xy, int yx, int yy);

static void raycast_octant_at(tile_map map, int y, int x, int radius, int row,
	float start_slope, float end_slope, int octant, raycast_fn *callback,
	void *context);

struct entity demo_new_snake(int y, int x);
void demo_place_torch(int y, int x);

void demo_floor_load_map(const char *filename)
{
//	FILE *mapfp = fopen(filename, "r");
	floor_map_generate(&demo_floor, CAVE);

	/* Make this not shit please. */
	for (size_t line = 0; line < MAP_LINES; ++line) {
		for (size_t col = 0; col < MAP_COLS; ++col) {
//			fscanf(mapfp, "%c", &demo_floor.map[line][col].token);
			demo_floor.map[line][col].light = 0;
			demo_floor.map[line][col].r = 51;
			demo_floor.map[line][col].g = 51;
			demo_floor.map[line][col].b = 51;

			if (demo_floor.map[line][col].token != '.') {
				demo_floor.map[line][col].g = 51;
				demo_floor.map[line][col].b = 51;
			}
		}
//		(void)fgetc(mapfp);
	}

//	fclose(mapfp);

	for (int i = 0; i < 20; ++i) {
		int y = rand() % MAP_LINES;
		int x = rand() % MAP_COLS;
		demo_place_torch(y, x);
	}
}

int drawn_to[MAP_LINES][MAP_COLS] = { 0 };

struct light_info {
	tile_map *map;
	float bright;
	int y, x;
	int r, g, b;
};

#include <assert.h>

def_raycast_fn(cast_light_at)
{
	if (drawn_to[y][x])
		return;
	struct light_info *info = context;
	drawn_to[y][x] = 1;
	int distance = sqrt(pow(y - info->y, 2) + pow(x - info->x, 2));
	const float dlight = info->bright / (distance + 1) / (distance + 1);
	struct tile tile = (*info->map)[y][x];
	if (y == info->y && x == info->x) {
		(*info->map)[y][x].light += info->bright;
	} else {
		(*info->map)[y][x].light += dlight;
//		assert((*info->map)[y][x].light > 0);
		(*info->map)[y][x].dr = min(info->r * dlight + tile.dr, 255);
		(*info->map)[y][x].dg = min(info->g * dlight + tile.dg, 255);
		(*info->map)[y][x].db = min(info->b * dlight + tile.db, 255);
	}
}

def_entity_fn(demo_player_update)
{
	int y = this->posy;
	int x = this->posx;
	float bright = player_lantern_on && player_fuel > 0 ? 0.5f : 0.1f;
	if (player_lantern_on) {
		if (player_fuel > 0) {
			player_fuel--;
		} else {
			player_lantern_on = false;
		}
	}
	//cast_light(this->floor->map, y, x, 6, 0.3f, this->r, this->g, this->b);
	raycast_at(this->floor->map, y, x, 6, &cast_light_at,
		&(struct light_info) {
			.map = &this->floor->map,
			.bright = bright, .y = y, .x = x,
			.r = this->r, .g = this->g, .b = this->b,
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
	raycast_at(this->floor->map, y, x, 6, &cast_light_at,
		&(struct light_info) {
			.map = &this->floor->map,
			.bright = 1.f, .y = y, .x = x, 
			.r = this->r, .g = this->g, .b = this->b,
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

#include <stdint.h>
#include <stdlib.h>

struct entity demo_new_torch(int y, int x)
{
	uint8_t r = rand() % 256;
	uint8_t g = rand() % 256;
	uint8_t b = rand() % 256;

	struct entity torch = {
		.r = 0xe2, .g = 0x58, .b = 0x22,
		//.r = r, .g = g, .b = b,
		.token = 't',
		.posy = y, .posx = x,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
		.blocks_light = 0,
	};

	return torch;
}

def_main_win_key_fn(place_torch)
{
	struct entity torch = demo_new_torch(player.posy, player.posx);
	struct entity *t = malloc(sizeof(torch));
	memcpy(t, &torch, sizeof(torch));
	floor_add_entity(cur_floor, t);
}

void demo_place_torch(int y, int x)
{
	struct entity snake = demo_new_snake(y, x);
	struct entity *s = malloc(sizeof(snake));
	memcpy(s, &snake, sizeof(snake));
	floor_add_entity(&demo_floor, s);
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
}

struct entity demo_new_snake(int y, int x)
{
	struct entity snake = {
		.r = 0xa, .g = 0xCA, .b = 0xa,
		.token = 's',
		.posy = y, .posx = x,
		.update = demo_snake_update,
		.destroy = NULL,
		.list = LIST_HEAD_INIT(snake.list),
		.floor = &demo_floor,
		.blocks_light = 0,
	};

	return snake;
}

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

static void cast_light(tile_map map, int y, int x, int radius, float bright,
	int r, int g, int b)
{
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 1, 0, 0, 1);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 0, 1, 1, 0);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 0, -1, 1, 0);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, -1, 0, 0, 1);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, -1, 0, 0, -1);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 0, -1, -1, 0);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 0, 1, -1, 0);
	cast_octant(map, y, x, radius, bright, r, g, b, 1, 1.0, 0.0, 1, 0, 0, -1);

	map[y][x].light += bright;
	/*
	map[y][x].dr = min(r * bright + tile.dr, 255);
	map[y][x].dg = min(g * bright + tile.dg, 255);
	map[y][x].db = min(b * bright + tile.db, 255);
	*/

	memset(drawn_to, 0, (sizeof(drawn_to[0][0]) * MAP_LINES * MAP_COLS));
}

static void cast_octant(tile_map map, int y, int x, int radius, float bright,
	int r, int g, int b, int row, float start_slope, float end_slope,
	int xx, int xy, int yx, int yy)
{
	if (start_slope < end_slope) {
		return;
	}

	float next_start_slope = start_slope;
	for (int i = row; i <= radius; ++i) {
		int blocked = 0;
		for (int dx = -i, dy = -i; dx <= 0; dx++) {
			float bl_slope = (dx - 0.5) / (dy + 0.5);
			float tr_slope = (dx + 0.5) / (dy - 0.5);
			if (start_slope < tr_slope) {
				continue;
			} else if (end_slope > bl_slope) {
				break;
			}

			int sax = dx * xx + dy * xy;
			int say = dx * yx + dy * yy;
			if ((sax < 0 && abs(sax) > x) ||
				(say < 0 && abs(say) > y)) {
				continue;
			}
			uint ax = x + sax;
			uint ay = y + say;
			if (!floor_map_in_bounds(ay, ax)) {
				continue;
			}

			uint radius2 = radius * radius;
			if ((uint)(dx * dx + dy * dy) < radius2 && !drawn_to[ay][ax]) {
				drawn_to[ay][ax] = 1;
				int distance = sqrt(dx * dx + dy * dy);
				const float dlight = bright / (distance + 1) / (distance + 1);
				struct tile tile = map[ay][ax];
				map[ay][ax].light = floor(dlight + tile.light);
				map[ay][ax].dr = min(r * dlight + tile.dr, 255);
				map[ay][ax].dg = min(g * dlight + tile.dg, 255);
				map[ay][ax].db = min(b * dlight + tile.db, 255);
			}

			struct tile tile = map[ay][ax];
			if (blocked) {
				if (!tile.walk || tile.entity) {
					next_start_slope = tr_slope;
					continue;
				} else {
					blocked = 0;
					start_slope = next_start_slope;
				}
			} else if (!tile.walk || tile.entity) {
				blocked = 1;
				next_start_slope = tr_slope;
				cast_octant(map, y, x, radius, bright, r, g, b,
					i + 1, start_slope, bl_slope, xx, xy,
					yx, yy);
			}
		}
		if (blocked) {
			break;
		}
	}
}

def_main_win_key_fn(demo_get_fuel)
{
	player_fuel += 10;
}
