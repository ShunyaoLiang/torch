#include "torch.h"

input_key_fn *input_keymap[] = {
	['h'] = player_move_left,
	['j'] = player_move_down,
	['k'] = player_move_up,
	['l'] = player_move_right,
	['y'] = player_move_upleft,
	['u'] = player_move_upright,
	['b'] = player_move_downleft,
	['n'] = player_move_downright,
	['f'] = player_attack,
	['t'] = place_torch,
	['e'] = player_toggle_lantern,
	['E'] = demo_get_fuel,
	[255] = NULL,
};
