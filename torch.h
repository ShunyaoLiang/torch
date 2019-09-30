#ifndef TORCH
#define TORCH

#include <tickit.h>
#include <stdint.h>

/* Viewport size. */
#define VIEW_LINES 23
#define VIEW_COLS  79

typedef unsigned int uint;

struct sprite {
	TickitPenRGB8 colour;
	char token;
};

#endif
