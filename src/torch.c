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
			ui_draw_at(12, 28, "lol you fucking died nerd", (struct ui_cell_attr){
				.fg = {0xff, 0xff, 0xff},
			});
			ui_flush();
			getchar();
			return;
		}

		ui_clear();
		draw();

		ui_flush();
	} while (event = ui_poll_event(), event.key != 'Q');
}

void menu_screen(void)
{
	ui_clear();
	ui_draw_at(9, 37, "Torch", (struct ui_cell_attr){
		.fg = {0xff, 0x50, 0x00},
	});
	ui_draw_at(12, 32, "Menu coming soon", (struct ui_cell_attr){
		.fg = {0xcc, 0xcc, 0xcc},
	});
	ui_flush();
	ui_poll_event();
}

int main(int argc, char *argv[])
{
#ifdef DEBUG
	debug_log = fopen("debug_log", "w");
#endif
	ui_init();
	floor_move_player(cur_floor, 50, 50);

	menu_screen();
	draw_init();

	event_loop(input_keymap, &draw_game);

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
	[','] = player_pick_up_item,
	[255] = NULL,
};
