#include "torch.h"

#include <tickit.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

TickitWindowEventFn main_win_on_key;
TickitWindowEventFn main_win_draw;

FILE *debug_log;

int main(void)
{
	srand(time(NULL));
	/* libtickit 0.3.2 only enables direct (true) colors when TERM is set to "xterm". */
	putenv("TERM=xterm");
	debug_log = fopen("debug_log", "w");

	demo_floor_load_map("map");
	INIT_LIST_HEAD(&demo_floor.entities);
	floor_add_entity(cur_floor, &player);

	demo_add_entities();

	Tickit *tickit_instance = tickit_new_stdio();
	
	TickitWindow *main_win = tickit_get_rootwin(tickit_instance);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_KEY, 0, main_win_on_key, NULL);
	tickit_window_bind_event(main_win, TICKIT_WINDOW_ON_EXPOSE, 0, main_win_draw, NULL);

	tickit_run(tickit_instance);

	tickit_window_close(main_win);

	tickit_unref(tickit_instance);

	return EXIT_SUCCESS;
}
