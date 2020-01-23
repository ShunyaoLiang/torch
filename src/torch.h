#ifndef TORCH
#define TORCH

#include "list.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define back(arr) (arr[sizeof(arr)/sizeof(*arr)])
#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)
#define clamp(v, lo, hi) ((v < lo) ? lo : (hi < v) ? hi : v)

typedef unsigned int uint;

extern FILE *debug_log;

/* Input */
#define def_input_key_fn(name) int name(void)
typedef def_input_key_fn(input_key_fn);

extern input_key_fn *input_keymap[];

/* Color */
struct color {
	uint8_t r, g, b;
};

struct color color_add(struct color c, struct color a);
struct color color_multiply_by(struct color c, float m);

int color_approximate_256(struct color c);

/* Entity */
struct entity;

#define def_entity_fn(name) void name(struct entity *this)
typedef def_entity_fn(entity_fn);

struct entity {
	struct color color;
	char token;
	int posy, posx;
	entity_fn *update;
	entity_fn *destroy;
	struct list_head list;
	struct floor *floor;
	uint blocks_light : 1;
};

int entity_move_pos(struct entity *e, int y, int x);
int entity_move_pos_rel(struct entity *e, int y, int x);

/* Player */
extern struct entity player;
extern bool player_lantern_on;
extern int player_fuel;
extern int player_torches;

def_input_key_fn(player_move_left);
def_input_key_fn(player_move_down);
def_input_key_fn(player_move_up);
def_input_key_fn(player_move_right);
def_input_key_fn(player_move_upleft);
def_input_key_fn(player_move_upright);
def_input_key_fn(player_move_downleft);
def_input_key_fn(player_move_downright);
def_input_key_fn(player_toggle_lantern);

/* Floor */
struct tile {
	struct color color;
	char token;
	float light;
	struct color dcolor;
	struct entity *entity;
	uint walk : 1;
};

bool tile_blocks_light(struct tile);

#define MAP_LINES 100
#define MAP_COLS  100

typedef struct tile tile_map[MAP_LINES][MAP_COLS];
typedef struct list_head entity_list;

struct floor {
	tile_map map;
	entity_list entities;
};

extern struct floor floors[];

extern struct floor *cur_floor;

enum floor_type {
	CAVE,
};

struct tile floor_map_at(struct floor *floor, int y, int x);
int         floor_map_in_bounds(int y, int x);
void        floor_map_clear_lights(void);
void        floor_map_generate(struct floor *floor, enum floor_type type);

int  floor_add_entity(struct floor *floor, struct entity *entity);
void floor_update_entities(struct floor *floor);

#define floor_for_each_tile(pos, floor) \
	for (pos = *floor->map; pos != back(floor->map); ++pos)

/* Draw */
#define VIEW_LINES 23
#define VIEW_COLS  79

#if 0
void draw_map(void);
void draw_entities(void);
#endif
void draw_shit(void);

/* Demo */
void demo_floor_load_map(const char *filename);

def_entity_fn(demo_player_destroy);
def_entity_fn(demo_player_update);

void demo_add_entities(void);

def_input_key_fn(place_torch);
def_input_key_fn(demo_get_fuel);

extern struct floor demo_floor;

/* Raycast */
typedef void raycast_callback_fn(struct tile *tile, int y, int x, void *context);

void raycast_at(struct floor *floor, int y, int x, int radius,
	raycast_callback_fn callback, void *context);

/* Random */
int rand(void);

#endif
