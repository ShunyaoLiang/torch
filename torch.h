#ifndef TORCH
#define TORCH

#include "list.h"

#include <tickit.h>
#include <stdint.h>

/* Viewport size. */
#define VIEW_LINES 23
#define VIEW_COLS  79

#define back(arr) (arr[sizeof(arr)/sizeof(*arr)])

/* Entity */
struct entity;

#define def_entity_fn(name) void name(struct entity *this)
typedef def_entity_fn(entity_fn);

struct entity {
	uint8_t r, g, b;
	char token;
	int posy, posx;
	entity_fn *update;
	entity_fn *destroy;
	struct list_head list;
};

void entity_move_pos(struct entity *e, int y, int x);
void entity_move_pos_rel(struct entity *e, int y, int x);

/* Floor */
struct tile {
	uint8_t r, g, b;
	char token;
	int light;
	int dr, dg, db;
	struct entity *on;
};

#define MAP_LINES 20
#define MAP_COLS  40

typedef struct tile tile_map[MAP_LINES][MAP_COLS];
typedef struct list_head entity_list;

struct floor {
	tile_map map;
	entity_list entities;
};

extern struct floor *cur_floor;

struct tile floor_map_at(struct floor *floor, int y, int x);
void        floor_map_set(struct floor *floor, int y, int x, struct tile tile);
int         floor_map_in_bounds(int y, int x);
void        floor_map_clear_lights(void);

void floor_add_entity(struct floor *floor, struct entity *entity);

#define floor_for_each_tile(pos, floor) \
	for (pos = *floor->map; pos != back(floor->map); ++pos)

#endif
