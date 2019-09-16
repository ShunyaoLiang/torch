#ifndef TORCH
#define TORCH

struct tile {
	char sprite;
};

struct entity {
	int posx;
	int posy;
};

#define MAP_LINES 20
#define MAP_COLS  40

struct dungeon {
	struct tile map[MAP_LINES][MAP_COLS];
};

#endif
