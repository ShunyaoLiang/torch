#include "torch.h"
#include "list.h"
#include "ui.h" 
#include <stdbool.h>

struct entity player = {
	.info = PLAYER | COMBAT | LIGHT_SOURCE,
	.color = {
		.r = 0xa5, .g = 0xa5, .b = 0xa5,
	},
	.combat = {
		.hp = 5, .hp_max = 5,
	},
	.token = "@",
	.posy = 50, .posx = 50,
	.update = demo_player_update,
	.destroy = NULL,
	.inventory = LIST_HEAD_INIT(player.inventory),
	.list = LIST_HEAD_INIT(player.list),
	.blocks_light = true,
	.floor = &floors[0],
};

bool player_lantern_on = false;
int player_fuel = 100;
int player_torches = 'z';

def_input_key_fn(player_move_left)
{
	return entity_move_pos_rel(&player, 0, -1);
}

def_input_key_fn(player_move_down)
{
	return entity_move_pos_rel(&player, 1, 0);
}

def_input_key_fn(player_move_up)
{
	return entity_move_pos_rel(&player, -1, 0);
}

def_input_key_fn(player_move_right)
{
	return entity_move_pos_rel(&player, 0, 1);
}

def_input_key_fn(player_move_upleft)
{
	return entity_move_pos_rel(&player, -1, -1);
}

def_input_key_fn(player_move_upright)
{
	return entity_move_pos_rel(&player, -1, 1);
}

def_input_key_fn(player_move_downleft)
{
	return entity_move_pos_rel(&player, 1, -1);
}

def_input_key_fn(player_move_downright)
{
	return entity_move_pos_rel(&player, 1, 1);
}

def_input_key_fn(player_attack)
{
	int y = player.posy;
	int x = player.posx;
	struct ui_event event = ui_poll_event();
	switch (event.key) {
	case 'h': x--; break;
	case 'j': y++; break;
	case 'k': y--; break;
	case 'l': x++; break;
	}

	struct entity *target = floor_map_at(cur_floor, y, x).entity;
	if (target) {
		return combat_do(&player.combat, &target->combat);
	}

	return 1;
}

def_input_key_fn(player_toggle_lantern)
{
	player_lantern_on = !player_lantern_on;
	return 0;
}

def_input_key_fn(player_pick_up_item)
{
	extern struct tile *floor_map_at_unsafe(struct floor *, int, int);
	struct tile *tile = floor_map_at_unsafe(player.floor, player.posx, player.posy);
	if (list_empty(&tile->items))
		return 1;

	int strcmp(const char *, const char *);
	void exit(int);
	if (!strcmp(((struct item *)tile->items.next)->name, "Sword")) {
		ui_clear();
		ui_flush();
		ui_quit();
		puts("a pretty message, you found the sword!");
		exit(0);
	}

	list_splice_tail_init(&tile->items, &player.inventory);
	return 0;
}
