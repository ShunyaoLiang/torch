#include "torch.h"
#include "ui.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *debug_log;

void log_init(void)
{
	char dir_name[] = "/tmp/torchXXXXXX";
	if (is_posix()) {
		if (!mkdtemp(dir_name))
			goto fail;
		if (chdir(dir_name))
			goto fail;
		if (!(debug_log = fopen("log", "w")))
			goto fail;
	}

	fprintf(stderr, "%s: log file located at %s", __func__, dir_name);

	return;

fail:
	fperror(stderr, __func__);
	exit(EXIT_FAILURE);
}

void menu_screen(void)
{
	int rows, cols;
	ui_dimensions(&rows, &cols);
	ui_clear();
	ui_flush();
	ui_draw_at_incremental(rows / 2 - 2, cols / 2 - 3, "Torch", (struct ui_cell_attr){
		.fg = {0xff, 0x50, 0x00},
	}, 50);
	ui_draw_at_incremental(rows / 2 + 1, cols / 2 - 12, "We couldn't afford a menu", (struct ui_cell_attr){
		.fg = {0xcc, 0xcc, 0xcc},
	}, 50);

	ui_poll_event();
}

void event_loop(int (*keymap[])(void), void (*draw)(void))
{
	struct ui_event event = { .key = 'e' };

	do {
		if (!keymap[event.key] || keymap[event.key]())
			continue;	

		floor_map_clear_lights();

		floor_update_entities(cur_floor);

		if (player.combat.hp <= 0) {
			int rows, cols;
			ui_dimensions(&rows, &cols);
			ui_clear();
			ui_flush();
			ui_draw_at_incremental(rows / 2, cols / 2 - 12, "lol you fucking died nerd", (struct ui_cell_attr){
				.fg = {0xac, 0x04, 0x04},
			}, 50);
			getchar();
			return;
		}

		ui_clear();
		draw();

		ui_flush();
	} while (event = ui_poll_event(), event.key != 'Q');
}

int main(int argc, char *argv[])
{
	log_init();
	ui_init();
	floor_move_player(cur_floor);

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
	['>'] = demo_descend_floor,
	['<'] = demo_ascend_floor,
	['E'] = demo_get_fuel,
	[','] = player_pick_up_item,
	[255] = NULL,
};
