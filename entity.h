#ifndef ENTITY
#define ENTITY

#include "torch.h"
#include "list.h"

struct entity {
	const char *name;
	int posx;
	int posy;
	struct sprite sprite;
	void (*update)(struct entity *this);
	int speed;
	int order;
	struct list_head list;
};

struct light {
	struct entity e;
	float emit;
	int duration;
};

#endif
