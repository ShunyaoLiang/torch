#include "torch.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#ifdef DEBUG
FILE *debug_log;
#endif

void torch_flicker(int signal);

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

int main(void)
{
#ifdef DEBUG
	debug_log = fopen("debug_log", "w");
#endif
	srand(time(NULL));

	INIT_LIST_HEAD(&floors[0].entities);
	floor_map_generate(&floors[0], CAVE);
	floor_add_entity(cur_floor, &player);

	ui_init();

	timer_t timer_id;
	timer_create(CLOCK_REALTIME, NULL, &timer_id);
	timer_settime(timer_id, 0, &(struct itimerspec) {
		.it_interval = { 0, 100000000 },
		.it_value = { 0, 100000000 },
	}, NULL);
	signal(SIGALRM, &torch_flicker);

	event_loop(input_keymap, &draw_shit);

	ui_quit();

	return EXIT_SUCCESS;
}
