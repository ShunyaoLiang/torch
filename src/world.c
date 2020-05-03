#include "torch.h"
#include "list.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

struct floor *cur_floor = &floors[0];

static void entity_place(enum entity_type type, struct floor *floor, int x, int y);

typedef int cell;
typedef cell cell_grid[MAP_LINES][MAP_COLS];

static void cave_populate_grid(cell_grid grid, float rate);
static void cave_iterate_grid(cell_grid grid, int birth, int survive);
static int  cave_cell_alive_neighbours(cell_grid grid, int y, int x);
static int  cave_cell_grid_at(cell_grid grid, int y, int x);
static void cave_floor_write_grid(struct floor *floor, cell_grid grid);

/*static*/ struct tile *floor_map_at_unsafe(struct floor *floor, int x, int y)
{
	if (floor_map_in_bounds(x, y))
		return &(floor->map)[y][x];
	else
		return NULL;
}

void floor_map_generate(struct floor *floor)
{
	if (floor->generated)
		return;

	if (floor->type == CAVE) {
		cell_grid grid = { 0 };
		cave_populate_grid(grid, 0.45f);

		for (int i = 0; i < 12; ++i) {
			cave_iterate_grid(grid, 5, 4);
		}

		cave_floor_write_grid(floor, grid);
		for (int i = 0; i < 10; ++i) {
			entity_place(
					random_int() % 2 ? ET_FLOATER : ET_SNAKE,
					floor, random_int() % 100, random_int() % 100
			);
		}

		int x, y;
		while (floor_map_at(floor, (x = random_int() % 100), (y = random_int() % 100)).blocks);
		struct tile *item_tile = floor_map_at_unsafe(floor, x, y);
		struct item *item = malloc(sizeof(struct item));
		*item = (struct item) {
			.name = "Sword",
			.token = "!",
			.color = { 0xff, 0xd7, 0x00 },
			.list = LIST_HEAD_INIT(item->list)
		};
		list_add_tail(&item->list, &item_tile->items);

		/* Place the staircases.
		   Upstairs */
		while (floor_map_at(floor, (x = random_int() % 100), (y = random_int() % 100)).blocks);
		struct tile *stair_tile = floor_map_at_unsafe(floor, x, y);
		stair_tile->type = TILE_UPSTAIR;
		stair_tile->token = "<";
		stair_tile->color = (struct color) { 0x01, 0xcd, 0xfe };
		floor->upstairs.x = x;
		floor->upstairs.y = y;

		/* Downstairs */
		stair_tile = floor_map_at_unsafe(floor, x + 2, y + 2);
		stair_tile->type = TILE_DOWNSTAIR;
		stair_tile->token = ">";
		stair_tile->color = (struct color) { 0x01, 0xcd, 0xfe };
		floor->downstairs.x = x + 2;
		floor->downstairs.y = y + 2;
	}

	floor->generated = true;
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
		pos->lighting.r = 0;
		pos->lighting.g = 0;
		pos->lighting.b = 0;
	}
}

int floor_add_entity(struct floor *floor, struct entity *entity)
{
	if (floor->map[entity->posy][entity->posx].entity)
		return -1;
	if (!floor_map_in_bounds(entity->posx, entity->posy))
		return -1;

	list_add(&entity->list, &floor->entities);
	entity->floor = floor;
	floor_map_at_unsafe(floor, entity->posx, entity->posy)->entity = entity;

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
	return tile.blocks || (tile.entity ? tile.entity->blocks_light : (bool)tile.entity);
}

void floor_move_player(struct floor *floor)
{
	if (!floor->generated) {
		INIT_LIST_HEAD(&floor->entities);
		floor_map_generate(floor);
	}

	/* Get staircase information. */
	int x, y;
	if (floor == player.floor->upstairs.floor) {
		/* If the player is moving up... */
		x = floor->downstairs.x;
		y = floor->downstairs.y;
	} else if (floor == player.floor->downstairs.floor) {
		/* If the player is moving down... */
		x = floor->upstairs.x;
		y = floor->upstairs.y;
	} else {
		/* Does the player's current floor even connect to the next floor? */
		return;
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

	cur_floor = floor;
}

#define cell_grid_for_each_cell(pos, grid) \
	for (pos = *grid; pos != back(grid); ++pos)

static void cave_populate_grid(cell_grid grid, float rate)
{
	for (int y = 0; y < MAP_LINES; ++y) {
		for (int x = 0; x < MAP_COLS; ++x) {
			if ((double)random_int() / INT_MAX < rate)
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
			t->color = (struct color) { 0x36, 0x38, 0x38 };
		} else {
			t->token = ".";
			t->blocks = false;
		}
		/* Create a floor_new() perhaps? */
		INIT_LIST_HEAD(&t->items);
		t->seen_as.token = " ";
		c++;
	}
}

struct floor floors[5] = {
	{ 
		.type = CAVE,
		.upstairs = { .floor = NULL, },
		.downstairs = { .floor = &floors[1], },
	},
	{ 
		.type = CAVE,
		.upstairs = { .floor = &floors[0], },
		.downstairs = { .floor = &floors[2], },
	},
	{ 
		.type = CAVE,
		.upstairs = { .floor = &floors[1], },
		.downstairs = { .floor = &floors[3], },
	},
	{ 
		.type = CAVE,
		.upstairs = { .floor = &floors[2], },
		.downstairs = { .floor = &floors[4], },
	},
	{ 
		.type = CAVE,
		.upstairs = { .floor = &floors[3], },
		.downstairs = { .floor = NULL, },
	},
};

int entity_move_pos(struct entity *entity, int y, int x)
{
	if (!floor_map_in_bounds(x, y))
		return -1;

	struct tile tile = floor_map_at(entity->floor, y, x);
	if (!tile.entity && !tile.blocks) {
		/* Slightly dangerous raw access. */
		entity->floor->map[entity->posy][entity->posx].entity = NULL;
		entity->floor->map[y][x].entity = entity;
		entity->posy = y;
		entity->posx = x;
		return 0;
	}

	return -1;
}

int entity_move_pos_rel(struct entity *entity, int y, int x)
{
	return entity_move_pos(entity, entity->posy + y, entity->posx + x);
}

static void entity_place(enum entity_type type, struct floor *floor, int x, int y)
{
	def_entity_fn(demo_snake_update);
	struct entity *entity = malloc(sizeof(struct entity));
	switch (type) {
	case ET_SNAKE:
		*entity = default_snake;
		break;
	case ET_FLOATER:
		*entity = default_floater;
		break;
	}
	entity->posx = x;
	entity->posy = y;
	entity->floor = floor;
	floor_add_entity(floor, entity);
}

int combat_do(struct combat *restrict c, struct combat *restrict target)
{
	target->hp--;
	return 0;
}
