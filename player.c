#include "torch.h"
#include "list.h"

struct entity player = {
	.r = 151, .g = 151, .b = 151,
	.token = '@',
	.posy = 13, .posx = 14,
	.update = demo_player_update,
	.destroy = NULL,
	.list = LIST_HEAD_INIT(player.list),
};

def_main_win_key_fn(player_move_left)
{
	entity_move_pos_rel(&player, 0, -1);
}

def_main_win_key_fn(player_move_down)
{
	entity_move_pos_rel(&player, 1, 0);
}

def_main_win_key_fn(player_move_up)
{
	entity_move_pos_rel(&player, -1, 0);
}

def_main_win_key_fn(player_move_right)
{
	entity_move_pos_rel(&player, 0, 1);
}
