#include "torch.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#ifdef DEBUG
FILE *debug_log;
#endif

int main(void)
{
	srand(time(NULL));
#ifdef DEBUG
	debug_log = fopen("debug_log", "w");
#endif

	INIT_LIST_HEAD(&demo_floor.entities);
	demo_floor_load_map("map");
	floor_add_entity(cur_floor, &player);

	demo_add_entities();

	ui_init();

	struct ui_event event = { .key = 'e' };

	do {
		if (!input_keymap[event.key] || input_keymap[event.key]())
			continue;

		floor_map_clear_lights();
		floor_update_entities(cur_floor);

		draw_map();
		draw_entities();

		ui_flush();
	} while (event = ui_poll_event(), event.key != 'Q');

	ui_quit();

	return EXIT_SUCCESS;
}
