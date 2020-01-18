#include "torch.h"

#include <stdlib.h>

struct color color_add(struct color c, struct color a) {
	return (struct color) {
		min(c.r + a.r, 255),
		min(c.g + a.g, 255),
		min(c.b + a.b, 255),
	};
}

struct color color_multiply_by(struct color c, float m) {
	return (struct color) {
		min(c.r * m, 255),
		min(c.g * m, 255),
		min(c.b * m, 255),
	};
}

int color_approximate_256(struct color c) {
	/* L. */
	return rand() % 256;
}
