#include "list.h"
#include "torch.h"

#include <stdlib.h>
#include <string.h>

typedef int cell;
typedef cell cell_grid[MAP_LINES][MAP_COLS];

#define cell_grid_for_each_cell(pos, grid) for (pos = *grid; pos != back(grid); ++pos)

static void intern_populate_grid(cell_grid grid, float rate);
static void intern_iterate_grid(cell_grid grid, int birth, int survive);
static int intern_cell_alive_neighbours(cell_grid grid, int y, int x);
static int intern_cell_grid_at(cell_grid grid, int y, int x);

static void intern_floor_write_grid(struct floor *floor, cell_grid grid);

void floor_map_generate(struct floor *floor, enum floor_type type)
{
	cell_grid grid = { 0 };
	intern_populate_grid(grid, 0.45f);

	for (int i = 0; i < 12; ++i) {
		intern_iterate_grid(grid, 4, 5);
	}

	intern_floor_write_grid(floor, grid);
}

static void intern_populate_grid(cell_grid grid, float rate)
{
	/*
		cell *pos;
		cell_grid_for_each_cell(pos, grid) {
			if (rand() % 100 / 100.f < rate)
				*pos = 1;
			else
				*pos = 0;
		}
	*/
	for (int y = 0; y < MAP_LINES; ++y) {
		for (int x = 0; x < MAP_COLS; ++x) {
			if (rand() % 100 / 100.f < rate)
				grid[y][x] = 1;
			else
				grid[y][x] = 0;
		};
	}
}

static void intern_iterate_grid(cell_grid grid, int birth, int survive)
{
	cell_grid new = { 0 };
	for (int y = 0; y < MAP_LINES; ++y)
		for (int x = 0; x < MAP_COLS; ++x) {
			int alive = intern_cell_alive_neighbours(grid, y, x);
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

static int intern_cell_alive_neighbours(cell_grid grid, int y, int x)
{
	int alive = 0;
	/*
		for (int i = -1; i < 2; ++i)
			for (int j = -1; j < 2; ++j) {
				if (i != 0 && j != 0 && grid[y + i][x + j])
					alive++;
			}
	*/

	intern_cell_grid_at(grid, y - 1, x - 1) ? alive++ : 0;
	intern_cell_grid_at(grid, y - 1, x) ? alive++ : 0;
	intern_cell_grid_at(grid, y - 1, x + 1) ? alive++ : 0;
	intern_cell_grid_at(grid, y, x - 1) ? alive++ : 0;
	intern_cell_grid_at(grid, y, x + 1) ? alive++ : 0;
	intern_cell_grid_at(grid, y + 1, x - 1) ? alive++ : 0;
	intern_cell_grid_at(grid, y + 1, x) ? alive++ : 0;
	intern_cell_grid_at(grid, y + 1, x + 1) ? alive++ : 0;

	return alive;
}

static int intern_cell_grid_at(cell_grid grid, int y, int x)
{
	if (floor_map_in_bounds(y, x))
		return grid[y][x];
	else
		return 1;
}

static void intern_floor_write_grid(struct floor *floor, cell_grid grid)
{
	struct tile *t;
	cell *c = *grid;
	floor_for_each_tile(t, floor)
	{
		if (*c) {
			t->token = '#';
			t->walk = 0;
		} else {
			t->token = '.';
			t->walk = 1;
		}
		c++;
	}
}

struct tile floor_map_at(struct floor *floor, int y, int x)
{
	if (floor_map_in_bounds(y, x))
		return (floor->map)[y][x];
	else
		return (struct tile) { .token = ' ' };
}

int floor_map_in_bounds(int y, int x)
{
	return y >= 0 && y < MAP_LINES && x >= 0 && x < MAP_COLS;
}

void floor_map_clear_lights(void)
{
	struct tile *pos;
	floor_for_each_tile(pos, cur_floor)
	{
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
	list_for_each_entry(pos, &floor->entities, list)
	{
		if (pos->update)
			pos->update(pos);
	}
}

bool tile_blocks_light(const struct tile *tile)
{
	return !tile->walk || (tile->entity && tile->entity->blocks_light);
}
