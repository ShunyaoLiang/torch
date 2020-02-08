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
	floor_init();
	floor_move_player(cur_floor, 66, 66);
	ui_init();
	draw_init();

	event_loop(input_keymap, &draw_shit);

	ui_quit();

	return EXIT_SUCCESS;
}
