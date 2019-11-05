#include "torch.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

struct floor demo_floor;
struct floor *cur_floor = &demo_floor;

void demo_floor_load_map(const char *filename)
{
	FILE *mapfd = fopen(filename, "r");
	floor_map_generate(&demo_floor, CAVE);

	/* Make this not shit please. */
	for (size_t line = 0; line < MAP_LINES; ++line) {
		for (size_t col = 0; col < MAP_COLS; ++col) {
//			fscanf(mapfd, "%c", &demo_floor.map[line][col].token);
			demo_floor.map[line][col].light = 0;
			demo_floor.map[line][col].r = 51;
			demo_floor.map[line][col].g = 51;
			demo_floor.map[line][col].b = 51;

			if (demo_floor.map[line][col].token != '.') {
				demo_floor.map[line][col].g = 150;
				demo_floor.map[line][col].b = 150;
			}
		}
		(void)fgetc(mapfd);
	}

	fclose(mapfd);
}

def_entity_fn(demo_player_update)
{
	int y = this->posy;
	int x = this->posx;
	struct tile (*map)[MAP_LINES][MAP_COLS] = &cur_floor->map;
	const int radius = 5;
	for (int drawy = y - radius; drawy <= y + radius; ++drawy) {
		for (int drawx = x - radius; drawx <= x + radius; ++drawx) {	
			const int distance = sqrt((drawx - x) * (drawx - x) + (drawy - y) * (drawy - y)); 
			if (!(distance <= radius) || !floor_map_in_bounds(drawy, drawx))
				continue;

			const float dlight = 0.3f / (distance + 1);
			(*map)[drawy][drawx].light += dlight;
			(*map)[drawy][drawx].dr += this->r * dlight;
			(*map)[drawy][drawx].dg += this->g * dlight;
			(*map)[drawy][drawx].db += this->b * dlight;
		}
	}
}

def_entity_fn(demo_torch_update)
{
	int y = (rand() % 3 - 1);
	int x = (rand() % 3 - 1);

	entity_move_pos_rel(this, y, x);

	y = this->posy;
	x = this->posx;
	struct tile (*map)[MAP_LINES][MAP_COLS] = &cur_floor->map;
	const int radius = 5;
	for (int drawy = y - radius; drawy <= y + radius; ++drawy) {
		for (int drawx = x - radius; drawx <= x + radius; ++drawx) {	
			const int distance = floor(sqrt((drawx - x) * (drawx - x) + (drawy - y) * (drawy - y))); 
			if (!(distance <= radius) || !floor_map_in_bounds(drawy, drawx))
				continue;

			const float dlight = 0.7f / (distance + 1);
			(*map)[drawy][drawx].light += dlight;
			(*map)[drawy][drawx].dr += this->r * dlight;
			(*map)[drawy][drawx].dg += this->g * dlight;
			(*map)[drawy][drawx].db += this->b * dlight;
		}
	}
}

def_entity_fn(demo_torch_destroy)
{

}

void demo_add_entities(void)
{
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

}

struct entity demo_new_torch(int y, int x)
{
	struct entity torch = {
		.r = 0xe2, .g = 0, .b = 0x22,
		.token = 't',
		.posy = y, .posx = x,
		.update = demo_torch_update,
		.destroy = demo_torch_destroy,
		.list = LIST_HEAD_INIT(torch.list),
		.floor = cur_floor,
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
