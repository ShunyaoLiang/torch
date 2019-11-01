#include "torch.h"

void entity_move_pos(struct entity *entity, int y, int x)
{
	if (floor_map_in_bounds(y, x) && !floor_map_at(entity->floor, y, x).entity) {
		/* Slightly dangerous raw access. */
		entity->floor->map[entity->posy][entity->posx].entity = NULL;
		entity->floor->map[y][x].entity = entity;
		entity->posy = y;
		entity->posx = x;
	}
}

void entity_move_pos_rel(struct entity *entity, int y, int x)
{
	entity_move_pos(entity, entity->posy + y, entity->posx + x);
}
