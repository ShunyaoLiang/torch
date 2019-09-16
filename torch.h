#ifndef TORCH
#define TORCH

typedef unsigned char flag;

struct tile {
	char sprite;
};

struct view {
	int lines;
	int cols;
};

struct entity {
	int posx;
	int posy;
};

#define in_range(n, low, high) (n >= low && n < high)

#endif
