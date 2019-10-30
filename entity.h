#ifndef ENTITY
#define ENTITY

#include "torch.h"
#include "list.h"

struct entity;

#define def_entity_update_fn(name) void name(struct entity *this)
typedef def_entity_update_fn(entity_update_fn);

struct entity {
	const char *name;
	struct point pos;
	struct sprite sprite;
	entity_update_fn *update;
	int speed;
	struct list_head list;
	int order;
};

extern struct entity player;

struct light_source {
	struct entity e;
	float emit;
	int duration;
};

#endif
