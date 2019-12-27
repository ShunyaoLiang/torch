#include "torch.h"
#include "list.h"

#include <stdbool.h>

struct entity player = {
	.r = 151, .g = 151, .b = 151,
	.token = '@',
	.posy = 13, .posx = 14,
	.update = demo_player_update,
	.destroy = NULL,
	.list = LIST_HEAD_INIT(player.list),
	.blocks_light = 1,
};

bool player_lantern_on = false;
int player_fuel = 100;
int player_torches = 0;

def_input_key_fn(player_move_left)
{
	return entity_move_pos_rel(&player, 0, -1);
}

def_input_key_fn(player_move_down)
{
	return entity_move_pos_rel(&player, 1, 0);
}

def_input_key_fn(player_move_up)
{
	return entity_move_pos_rel(&player, -1, 0);
}

def_input_key_fn(player_move_right)
{
	return entity_move_pos_rel(&player, 0, 1);
}

def_input_key_fn(player_move_upleft)
{
	return entity_move_pos_rel(&player, -1, -1);
}

def_input_key_fn(player_move_upright)
{
	return entity_move_pos_rel(&player, -1, 1);
}

def_input_key_fn(player_move_downleft)
{
	return entity_move_pos_rel(&player, 1, -1);
}

def_input_key_fn(player_move_downright)
{
	return entity_move_pos_rel(&player, 1, 1);
}

def_input_key_fn(player_toggle_lantern)
{
	player_lantern_on = !player_lantern_on;
	return 0;
}
