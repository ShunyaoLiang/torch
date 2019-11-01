#include "torch.h"
#include "list.h"

#include <stdlib.h>
#include <string.h>

typedef short cell;
typedef cell cell_grid[MAP_LINES][MAP_COLS];

#define cell_grid_for_each_cell(pos, grid) \
	for (cell *pos = *grid; pos != back(grid); ++pos)

static void __populate_grid(cell_grid grid, float rate);
static void __iterate_grid(cell_grid grid, int birth, int survive);
static int __cell_alive_neighbours(cell_grid grid, int y, int x);

static void __floor_write_grid(struct floor *floor, cell_grid grid);

void floor_map_generate(struct floor *floor)
{
	cell_grid grid;
	__populate_grid(grid, 0.45f);

	for (int i = 0; i < 12; ++i) {
		__iterate_grid(&grid, 4, 5);
	}

	__floor_write_grid(floor, grid);	
}

static void __populate_grid(cell_grid grid, float rate)
{
	cell *pos;
	cell_grid_for_each_cell(pos, grid) {
		if (rand() % 100 / 100 < rate)
			*pos = 1;
		else
			*pos = 0;
	}
}

static void __iterate_grid(cell_grid grid, int birth, int survive)
{
	cell_grid new = { 0 };
	for (int y = 0; y < MAP_LINES; ++y)
		for (int x = 0; x < MAP_COLS; ++x) {
			int alive = __cell_alive_neighbours(grid, y, x);
			if (grid[y][x])
				if (alive >= survive)
					new[y][x] = 1;
			else
				if (alive >= birth)
					new[y][x] = 1;
		}

	memcpy(new, grid, sizeof(new));
}

static int __cell_alive_neighbours(cell_grid grid, int y, int x)
{
	int alive = 0;
	for (int i = -1; i < 2; ++i)
		for (int j = -1; j < 2; ++j) {
			if (i != 0 && j != 0 && grid[y + i][x + j])
				alive++;
		}

	return alive;
}

static void __floor_write_grid(struct floor *floor, cell_grid grid)
{
	struct tile *t;
	cell *c = grid;
	floor_for_each_tile(t, floor) {
		if (*c)
			t->token = '#';
		else
			t->token = ' ';
		c++;
	}
}

struct tile floor_map_at(struct floor *floor, int y, int x)
{
	if (floor_map_in_bounds(y, x))
		return (floor->map)[y][x];
	else
		return (struct tile){ 0 };
}

void floor_map_set(struct floor *floor, int y, int x, struct tile tile)
{
	(floor->map)[y][x] = tile;
}

int floor_map_in_bounds(int y, int x)
{
	return y >= 0 && y < MAP_LINES && x >= 0 && x < MAP_COLS;
}

void floor_map_clear_lights(void)
{
	struct tile *pos;
	floor_for_each_tile(pos, cur_floor) {
		pos->light = 0;
		pos->dr = 0;
		pos->dg = 0;
		pos->db = 0;
	}
}

void floor_add_entity(struct floor *floor, struct entity *entity)
{
	list_add(&entity->list, &floor->entities);
	entity->floor = floor;
	floor->map[entity->posy][entity->posx].entity = entity;
}

void floor_update_entities(struct floor *floor)
{
	struct entity *pos;
	list_for_each_entry(pos, &floor->entities, list) {
		if (pos->update)
			pos->update(pos);
	}
}
