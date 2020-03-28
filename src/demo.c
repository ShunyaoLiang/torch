#include "torch.h"
#include "list.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct entity demo_new_snake(int y, int x);
void demo_place_snake(int y, int x);

int drawn_to[MAP_LINES][MAP_COLS] = { 0 };

struct light_info {
	tile_map *map;
	float bright;
	int y, x;
	struct color color;
};

#include <assert.h>

void cast_light_at(struct tile *tile, int x, int y, void *context)
{
	if (drawn_to[y][x])
		return;
	struct light_info *info = context;
	drawn_to[y][x] = 1;
	int distance = sqrt(pow(y - info->y, 2) + pow(x - info->x, 2));
	const float dlight = info->bright / (2 * distance + 1);
	if (y == info->y && x == info->x) {
		(*info->map)[y][x].light += info->bright;
	} else {
		(*info->map)[y][x].light += dlight;
//		assert((*info->map)[y][x].light > 0);
		/*                        = info->r * dlight + tile.lighting */
		(*info->map)[y][x].lighting = color_add(color_multiply_by(info->color, dlight), tile->lighting);
	}
	
	tile->seen = true;
}

int max_radius(struct entity *this, float bright) 
{
    int max_rgb = max(max(this->color.r, this->color.b), this->color.g);
    return (max_rgb * bright - LIGHT_SENSITIVITY) / (2 * LIGHT_SENSITIVITY);
}

def_entity_fn(demo_player_update)
{
	int y = this->posy;
	int x = this->posx;
	float bright = 0.5f;
	if (player_lantern_on) {
		if (player_fuel > 0) {
			player_fuel--;
		} else {
			player_lantern_on = false;
		}
	}

	static int radius = -1;
	if (radius == -1) {
		radius = max_radius(this, bright);
	}
	//cast_light(this->floor->map, y, x, 6, 0.3f, this->r, this->g, this->b);
	raycast_at(this->floor, x, y, radius, &cast_light_at,
		&(struct light_info) {
			.map = &this->floor->map,
			.bright = bright, .y = y, .x = x,
			.color = this->color,
		});
	memset(drawn_to, 0, (sizeof(drawn_to[0][0]) * MAP_LINES * MAP_COLS));
}

def_entity_fn(demo_torch_update)
{
	int y = (random_int() % 3 - 1);
	int x = (random_int() % 3 - 1);

//	entity_move_pos_rel(this, y, x);

	y = this->posy;
	x = this->posx;

	int flicker = (random_int() % 3 - 1);
	float bright = 1.f + flicker * 0.2f;

	static int radius = -1;
	if (radius == -1) {
		radius = max_radius(this, bright);
	}
	//cast_light(this->floor->map, y, x, 6, 1.f, this->r, this->g, this->b);
	raycast_at(this->floor, x, y, radius, &cast_light_at,
		&(struct light_info) {
			.map = &this->floor->map,
			.bright = bright, .y = y, .x = x, 
			.color = this->color,
		});
	memset(drawn_to, 0, (sizeof(drawn_to[0][0]) * MAP_LINES * MAP_COLS));
}

def_entity_fn(demo_torch_destroy)
{

}

void torch_flicker(int signal)
{
	if (!ui_polling)
		return;

	floor_map_clear_lights();
	struct entity *pos;
	list_for_each_entry(pos, &cur_floor->entities, list) {
		if ((pos->info & LIGHT_SOURCE) == LIGHT_SOURCE)
			pos->update(pos);
	}
	
	ui_clear();
	draw_game();
	ui_flush();
}

struct entity demo_new_torch(int y, int x)
{
	struct entity torch = {
		.info = LIGHT_SOURCE,
		.color = {
			.r = 0xe2, .g = 0x58, .b = 0x22,
		},
		.token = "!",
		.posy = y, .posx = x,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
		.blocks_light = false,
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
	if (!floor_map_in_bounds(x, y))
		return -1;
	struct entity torch = demo_new_torch(y, x);
	struct entity *t = malloc(sizeof(torch));
	memcpy(t, &torch, sizeof(torch));
	return floor_add_entity(cur_floor, t);
}

def_input_key_fn(demo_get_fuel)
{
	player_fuel += 10000;
	return 0;
}

def_input_key_fn(demo_descend_floor)
{
	int floor_index = player.floor - floors - 1;
	if (floor_index >= 0 && floor_index < 5) {
		floor_move_player(&floors[floor_index], player.posx, player.posy);
		return 0;
	} else {
		return 1;
	}
}

def_input_key_fn(demo_ascend_floor)
{
	int floor_index = player.floor - floors + 1;
	if (floor_index < 5) {
		floor_move_player(&floors[floor_index], player.posx, player.posy);
		return 0;
	} else {
		return 1;
	}
}

void demo_place_snake(int y, int x)
{
	struct entity snake = demo_new_snake(y, x);
	struct entity *s = malloc(sizeof(snake));
	memcpy(s, &snake, sizeof(snake));
	floor_add_entity(cur_floor, s);
}

def_entity_fn(demo_snake_update)
{
	if (this->combat.hp <= 0) {
		__list_del_entry(&this->list);
		this->floor->map[this->posy][this->posx].entity = NULL;
		free(this);
		return;
	}

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
		combat_do(&this->combat, &player.combat);
	}
}


struct entity demo_new_snake(int y, int x)
{
	struct entity snake = {
		.color = {
			.r = 0x19, .g = 0x19, .b = 0x8c,
		},
		.combat = {
			.hp = 1, .hp_max = 1,
		},
		.token = "S",
		.posy = y, .posx = x,
		.update = demo_snake_update,
		.destroy = NULL,
		.list = LIST_HEAD_INIT(snake.list),
		.floor = &floors[0],
		.blocks_light = false,
	};

	return snake;
}
