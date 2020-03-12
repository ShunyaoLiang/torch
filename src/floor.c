#include "torch.h"
#include "list.h"

#include <stdlib.h>
#include <string.h>

struct floor *cur_floor = &floors[0];

typedef int cell;
typedef cell cell_grid[MAP_LINES][MAP_COLS];

static void cave_populate_grid(cell_grid grid, float rate);
static void cave_iterate_grid(cell_grid grid, int birth, int survive);
static int  cave_cell_alive_neighbours(cell_grid grid, int y, int x);
static int  cave_cell_grid_at(cell_grid grid, int y, int x);
static void cave_floor_write_grid(struct floor *floor, cell_grid grid);

void floor_map_generate(struct floor *floor)
{
	if (floor->type == CAVE) {
		cell_grid grid = { 0 };
		cave_populate_grid(grid, 0.45f);

		for (int i = 0; i < 12; ++i) {
			cave_iterate_grid(grid, 5, 4);
		}

		cave_floor_write_grid(floor, grid);	

		demo_place_snake(60, 60);
	}
}

struct tile floor_map_at(struct floor *floor, int y, int x)
{
	if (floor_map_in_bounds(x, y))
		return (floor->map)[y][x];
	else
		return (struct tile){ .token = " " };
}

int floor_map_in_bounds(int x, int y)
{
	return x >= 0 && x < MAP_COLS && y >= 0 && y < MAP_LINES;
}

void floor_map_clear_lights(void)
{
	struct tile *pos;
	floor_for_each_tile(pos, cur_floor) {
		pos->light = 0;
		pos->dcolor.r = 0;
		pos->dcolor.g = 0;
		pos->dcolor.b = 0;
	}
}

int floor_add_entity(struct floor *floor, struct entity *entity)
{
	if (floor->map[entity->posy][entity->posx].entity)
		return -1;
	list_add(&entity->list, &floor->entities);
	entity->floor = floor;
	floor->map[entity->posy][entity->posx].entity = entity;
	return 0;
}

void floor_update_entities(struct floor *floor)
{
	struct entity *pos;
	list_for_each_entry(pos, &floor->entities, list) {
		if (pos->update)
			pos->update(pos);
	}
}

bool tile_blocks_light(struct tile tile)
{
	return tile.blocks || (tile.entity ? tile.entity->blocks_light : tile.entity);
}

void floor_init(void)
{
	INIT_LIST_HEAD(&floors[0].entities);
	floor_map_generate(&floors[0]);
	floor_add_entity(cur_floor, &player);

	//XXX
	INIT_LIST_HEAD(&floors[1].entities);
	floor_map_generate(&floors[1]);

	floor_move_player(cur_floor, 66, 66);
}

static struct tile *floor_map_at_unsafe(struct floor *floor, int x, int y)
{
	if (floor_map_in_bounds(x, y))
		return &(floor->map)[y][x];
	else
		return NULL;
}

void floor_move_player(struct floor *floor, int x, int y)
{
	cur_floor = floor;

	if (!floor->map) {
		INIT_LIST_HEAD(&floor->entities);
		floor_map_generate(floor);
	}

	/* Update tile information. */
	floor_map_at_unsafe(player.floor, player.posx, player.posy)->entity = NULL;
	floor_map_at_unsafe(floor, x, y)->entity = &player;
	/* Update player information. */
	player.floor = floor;
	player.posx = x;
	player.posy = y;
	/* Move the list entry. */
	list_move_tail(&player.list, &floor->entities);
}

#define cell_grid_for_each_cell(pos, grid) \
	for (pos = *grid; pos != back(grid); ++pos)

static void cave_populate_grid(cell_grid grid, float rate)
{
	for (int y = 0; y < MAP_LINES; ++y) {
		for (int x = 0; x < MAP_COLS; ++x) {
			if (random_int() % 100 / 100.f < rate)
				grid[y][x] = 1;
			else
				grid[y][x] = 0;
		};
	}
}

static void cave_iterate_grid(cell_grid grid, int birth, int survive)
{
	cell_grid new = { 0 };
	for (int y = 0; y < MAP_LINES; ++y)
		for (int x = 0; x < MAP_COLS; ++x) {
			int alive = cave_cell_alive_neighbours(grid, y, x);
			if (grid[y][x]) {
				if (alive >= survive)
					new[y][x] = 1;
			} else {
				if (alive >= birth)
					new[y][x] = 1;
			}
		}

//	memcpy(new, grid, sizeof(new));
	for (int y = 0; y < MAP_LINES; ++y)
		for (int x = 0; x < MAP_COLS; ++x) {
			grid[y][x] = new[y][x];
		}
}

static int cave_cell_alive_neighbours(cell_grid grid, int y, int x)
{
	int alive = 0;

	cave_cell_grid_at(grid, y - 1, x - 1) ? alive++ : 0;
	cave_cell_grid_at(grid, y - 1, x) ? alive++ : 0;
	cave_cell_grid_at(grid, y - 1, x + 1) ? alive++ : 0;
	cave_cell_grid_at(grid, y, x - 1) ? alive++ : 0;
	cave_cell_grid_at(grid, y, x + 1) ? alive++ : 0;
	cave_cell_grid_at(grid, y + 1, x - 1) ? alive++ : 0;
	cave_cell_grid_at(grid, y + 1, x) ? alive++ : 0;
	cave_cell_grid_at(grid, y + 1, x + 1) ? alive++ : 0;
	
	return alive;
}

static int cave_cell_grid_at(cell_grid grid, int y, int x)
{
	if (floor_map_in_bounds(x, y))
		return grid[y][x];
	else
		return 1;
}

static void cave_floor_write_grid(struct floor *floor, cell_grid grid)
{
	struct tile *t;
	cell *c = *grid;
	floor_for_each_tile(t, floor) {
		if (*c) {
			t->token = "#";
			t->blocks = true;
		} else {
			t->token = ".";
			t->blocks = false;
		}
		c++;
	}
}

struct floor floors[2] = {
	{ .type = CAVE },
	{ .type = CAVE },
};
