#ifndef BLEND
#define BLEND

#include "floor.h"
#include "torch.h"

#include <stdint.h>
#include <tickit.h>

struct blend_data {
	int r, g, b;
	float light;
};

extern struct blend_data blend_buffer[MAP_LENGTH][MAP_WIDTH];

struct blend_data blend_buffer_at(int y, int x);
void blend_buffer_add(int y, int x, int r, int g, int b, float light);
void blend_buffer_flush_light(void);
void blend_buffer_clear(void);

TickitPenRGB8 blend_sprite(struct blend_data blend, struct sprite sprite);

#endif
