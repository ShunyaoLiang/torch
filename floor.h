#ifndef FLOOR
#define FLOOR

#include "torch.h"
#include "list.h"
#include "entity.h"

struct tile {
	struct sprite sprite;
	float light;
};

#define MAP_LINES 20
#define MAP_COLS  40

struct floor_map {
	struct tile tiles[MAP_LINES][MAP_COLS];
//	struct point *corners;
};

typedef struct list_head entity_list;

struct floor {
	struct floor_map map;
	entity_list entities;
};

extern struct floor *cur_floor;

void floor_add_entity(struct floor *floor, struct entity *new);

#endif
