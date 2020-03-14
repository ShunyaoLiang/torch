#include "torch.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef DEBUG
FILE *debug_log;
#endif

void event_loop(int (*keymap[])(void), void (*draw)(void))
{
	struct ui_event event = { .key = 'e' };

	do {
		if (!keymap[event.key] || keymap[event.key]())
			continue;	

		floor_map_clear_lights();

		floor_update_entities(cur_floor);

		if (player.combat.hp <= 0) {
			ui_clear();
			ui_flush();
			ui_quit();
			puts("lol you fucking died nerd");
			exit(0);
		}
#if 0
		draw_map();
		draw_entities();
#endif
		ui_clear();
		draw();

		ui_flush();
	} while (event = ui_poll_event(), event.key != 'Q');
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
	debug_log = fopen("debug_log", "w");
#endif
	ui_init();
	floor_init();
	draw_init();

	event_loop(input_keymap, &draw_shit);

	ui_quit();

	return EXIT_SUCCESS;
}

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
	['J'] = demo_descend_floor,
	['K'] = demo_ascend_floor,
	['E'] = demo_get_fuel,
	[255] = NULL,
};
