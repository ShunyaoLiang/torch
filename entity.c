#include "torch.h"

void entity_move_pos(struct entity *entity, int y, int x)
{
	struct tile tile = floor_map_at(cur_floor, y, x);
	if (floor_map_in_bounds(y, x) && !tile.on) {
		floor_map_set(cur_floor, y, x, (struct tile){
			.r = tile.r, .g = tile.g, .b = tile.b,
			.token = tile.token,
			.light = tile.light,
			.dr = tile.dr, .dg = tile.dg, .db = tile.db,
			.on = entity,
		});
		tile = floor_map_at(cur_floor, entity->posy, entity->posx);
		floor_map_set(cur_floor, entity->posy, entity->posx, (struct tile){
			.r = tile.r, .g = tile.g, .b = tile.b,
			.token = tile.token,
			.light = tile.light,
			.dr = tile.dr, .dg = tile.dg, .db = tile.db,
			.on = NULL,
		});

		entity->posy = y;
		entity->posx = x;
	}
}

void entity_move_pos_rel(struct entity *entity, int y, int x)
{
	entity_move_pos(entity, entity->posy + y, entity->posx + x);
}
