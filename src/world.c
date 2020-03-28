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
	if (floor->type == CAVE) {
		cell_grid grid = { 0 };
		cave_populate_grid(grid, 0.45f);

		for (int i = 0; i < 12; ++i) {
			cave_iterate_grid(grid, 5, 4);
		}

		cave_floor_write_grid(floor, grid);
		for (int i = 0; i < 10; ++i)
			entity_place(SNAKE, floor, random_int() % 100, random_int() % 100);

		int x, y;
		while (floor_map_at(floor, (x = random_int() % 100), (y = random_int() % 100)).blocks);
		struct tile *item_tile = floor_map_at_unsafe(floor, x, y);
		struct item *item = malloc(sizeof(struct item));
		*item = (struct item) {
			.name = "Sword",
			.token = "/",
			.color = { 0x55, 0x66, 0x77 },
			.list = LIST_HEAD_INIT(item->list)
		};
		list_add_tail(&item->list, &item_tile->items);
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

void floor_move_player(struct floor *floor, int x, int y)
{
	cur_floor = floor;

	if (!floor->generated) {
		floor->generated = true;
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
	{ .type = CAVE },
	{ .type = CAVE },
	{ .type = CAVE },
	{ .type = CAVE },
	{ .type = CAVE },
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
	case SNAKE:
		*entity = (struct entity) {
			.info = COMBAT,
			.color = {
				.r = 0x19, .g = 0x19, .b = 0x8c,
			},
			.combat = {
				.hp = 1, .hp_max = 1,
			},
			.token = "S",
			.posx = x, .posy = y,
			.update = demo_snake_update,
			.destroy = NULL,
			.inventory = LIST_HEAD_INIT(entity->inventory),
			.floor = floor,
			.blocks_light = false,
		};
	}
	floor_add_entity(floor, entity);
}

int combat_do(struct combat *restrict c, struct combat *restrict target)
{
	target->hp--;
	return 0;
}
