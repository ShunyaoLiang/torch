#include "torch.h"

int entity_move_pos(struct entity *entity, int y, int x)
{
	struct tile tile = floor_map_at(entity->floor, y, x);
	if (floor_map_in_bounds(y, x) && !tile.entity && tile.walk) {
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
