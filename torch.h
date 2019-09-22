#ifndef TORCH
#define TORCH

#include "list.h"

struct tile {
	char sprite;
};

struct entity {
	int posx;
	int posy;

	char sprite;

	struct list_head list;

	void (*update)(struct entity *this);
};

#define MAP_LINES 20
#define MAP_COLS  40

struct dungeon {
	struct tile tile_map[MAP_LINES][MAP_COLS];
	float light_map[MAP_LINES][MAP_COLS];

	struct list_head entities;
};

#endif
