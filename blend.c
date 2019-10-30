#include "blend.h"

#include <tickit.h>
#include <string.h>

struct blend_data blend_buffer[MAP_LENGTH][MAP_WIDTH];

struct blend_data blend_buffer_at(int y, int x)
{
	if (y >= 0 && y < MAP_LENGTH && x >= 0 && x < MAP_WIDTH) {
		return blend_buffer[y][x];
	} else {
		return (struct blend_data) { 0 };
	}
}

void blend_buffer_add(int y, int x, int r, int g, int b, float light)
{
	if (y >= 0 && y < MAP_LENGTH && x >= 0 && x < MAP_WIDTH) {
		blend_buffer[y][x].r += r * light;
		blend_buffer[y][x].g += g * light;
		blend_buffer[y][x].b += b * light;
		blend_buffer[y][x].light += light;
	}
}

void blend_buffer_flush_light(void)
{
	for (int y = 0; y < MAP_LENGTH; ++y) {
		for (int x = 0; x < MAP_WIDTH; ++x) {
			cur_floor->map[y][x].light = blend_buffer[y][x].light;
		}
	}
}

void blend_buffer_clear(void)
{
	memset(blend_buffer, 0, sizeof(blend_buffer));
}

TickitPenRGB8 blend_sprite(struct blend_data blend, struct sprite sprite)
{
#define min(x, y) (x < y ? x : y)

	return (TickitPenRGB8) {
		.r = min(blend.r + sprite.colour.r * blend.light, 255),
		.g = min(blend.g + sprite.colour.g * blend.light, 255),
		.b = min(blend.b + sprite.colour.b * blend.light, 255),
	};
}
