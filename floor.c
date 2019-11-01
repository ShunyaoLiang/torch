#include "torch.h"
#include "list.h"

struct tile floor_map_at(struct floor *floor, int y, int x)
{
	if (floor_map_in_bounds(y, x))
		return (floor->map)[y][x];
	else
		return (struct tile){ 0 };
}

void floor_map_set(struct floor *floor, int y, int x, struct tile tile)
{
	(floor->map)[y][x] = tile;
}

int floor_map_in_bounds(int y, int x)
{
	return y >= 0 && y < MAP_LINES && x >= 0 && x < MAP_COLS;
}

void floor_map_clear_lights(void)
{
	struct tile *pos;
	floor_for_each_tile(pos, cur_floor) {
		pos->light = 0;
		pos->dr = 0;
		pos->dg = 0;
		pos->db = 0;
	}
}

void floor_add_entity(struct floor *floor, struct entity *entity)
{
	list_add(&entity->list, &floor->entities);
	entity->floor = floor;
	floor->map[entity->posy][entity->posx].entity = entity;
}

void floor_update_entities(struct floor *floor)
{
	struct entity *pos;
	list_for_each_entry(pos, &floor->entities, list) {
		if (pos->update)
			pos->update(pos);
	}
}
