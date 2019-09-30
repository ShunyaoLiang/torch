#ifndef FLOOR
#define FLOOR

#include "torch.h"
#include "list.h"
#include "entity.h"

struct tile {
	const char *what;
	struct sprite sprite;
	float light;
	uint solid : 1;
	struct entity *above;
};

struct floor {
//	struct tile **map;
//	int width, length;
#define MAP_LENGTH 20
#define MAP_WIDTH  40
	struct tile map[MAP_LENGTH][MAP_WIDTH];
	struct list_head entities;
};

extern struct floor *cur_floor;

#endif
