#ifndef TORCH
#define TORCH

#include "list.h"

#include <tickit.h>
#include <stdint.h>

/* Viewport size. */
#define VIEW_LINES 23
#define VIEW_COLS  79

typedef unsigned int uint;

struct point {
	int y, x;
};

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

typedef struct list_head entity_list;

struct tile {
	uint8_t r, g, b;
	char token;
	int light;
	int dr, dg, db;
	struct entity *on;
};

#define MAP_LINES 20
#define MAP_COLS  40

struct floor {
	struct tile tile_map[MAP_LINES][MAP_COLS];
	entity_list entities;
};

extern struct floor *cur_floor;

#endif
