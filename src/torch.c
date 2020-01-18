#include "torch.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef DEBUG
FILE *debug_log;
#endif

int main() {
	srand(time(NULL));
#ifdef DEBUG
	debug_log = fopen("debug_log", "w");
#endif

	INIT_LIST_HEAD(&floors[0].entities);
	demo_floor_load_map("map");
	floor_add_entity(cur_floor, &player);

	demo_add_entities();

	ui_init();

	struct ui_event event = { .key = 'e' };

	do {
		if (!input_keymap[event.key] || input_keymap[event.key]()) continue;

		floor_map_clear_lights();
		floor_update_entities(cur_floor);
#if 0
		draw_map();
		draw_entities();
#endif
		ui_clear();
		draw_shit();

		ui_flush();
	} while (event = ui_poll_event(), event.key != 'Q');

	ui_quit();

	return EXIT_SUCCESS;
}
