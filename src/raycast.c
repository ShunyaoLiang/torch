#include "torch.h"

#include <stdbool.h>

static void raycast_octant_at(struct floor *floor, int x, int y, int radius,
	int row, float start_slope, float end_slope, int octant,
	raycast_callback_fn callback, void *context);

static void transform_point_by_octant(int x, int y, int dx, int dy, int octant,
	int *ax, int *ay);

/* Hides hard-coded initial values for raycast_octant_at() */
#define RAYCAST_OCTANT_AT(map, x, y, radius, octant, callback, context) do { \
	raycast_octant_at(map, x, y, radius, 1, 1.0, 0.0, octant, callback, \
		context); \
} while (0);

/* Uses Björn Bergström's Recursive Shadowcasting Algorithm, which 
   divides the map into eight congruent triangular octants, and calculates each
   octant individually. */
void raycast_at(struct floor *floor, int x, int y, int radius,
	raycast_callback_fn callback, void *context)
{
	/* raycast_octant_at() does not cast on the origin (y, x). */
	callback(&floor->map[y][x], x, y, context);

	for (int octant = 0; octant < 8; ++octant)
		RAYCAST_OCTANT_AT(floor, x, y, radius, octant, callback, context);
}

/* Iterates through each tile, left to right, of each row of a given octant,
   recursing in each gap between blocking tiles, except for the last one.

   	7  --....-- - = visible tile     --....--
   	6   --...-- . = hidden tile       --...--
   	5    --##-- # = blocking tile      --!#-- ! = function recurses here
   	4     -----                         -----
   	3      ----                          ----
   	2       ---                           ---
   	1        --                            --
   	          @ @ = the centre              @

   Note: the algorithm uses an unorthodox understanding of coordinate gradients,
   calculating them as dx / dy rather than dy / dx. Consequently, as a line
   approaches the bottom left, its slope approaches infinity, and as a line
   approaches the top right, its slope approaches zero. */
static void raycast_octant_at(struct floor *floor, int x, int y, int radius,
	int row, float start_slope, float end_slope, int octant,
	raycast_callback_fn callback, void *context)
{
	if (start_slope < end_slope)
		return;

	/* This function is called recursively on each gap in blocking tiles
	   except for the last one. next_start_slope contains the slope of the
	   top right corner of the rightmost blocking tile. */
	float next_start_slope = start_slope;
	for (; row <= radius; ++row) {
		/* Doesn't do anything. Here for clarity. */
		start_slope = next_start_slope;
		/* Contains whether or not the last tile blocked light. Used
		   to determine when to recurse. */
		bool blocked = false;
		/* Contains the vector of the current tile from the centre. */
		int dx, dy;
		/* This implementation pretends its always calculating the
		   zeroth octant, then applies a transformation. */
		for (dx = -row, dy = -row; dx <= 0; ++dx) {
			/* Slope of the bottom left and top right corners
			   of the current tile from the centre, respectively */
			float bl_slope = (dx - 0.5) / (dy + 0.5);
			float tr_slope = (dx + 0.5) / (dy - 0.5);
			/* Accounts for mathematical edgecases. */
			if (start_slope < tr_slope) {
				continue;
			} else if (end_slope > bl_slope) {
				break;
			}

			/* Contains the actual coordinate of the current tile. */
			int ax, ay;
			transform_point_by_octant(x, y, dx, dy, octant, &ax, &ay);
			if (!floor_map_in_bounds(ay, ax))
				continue;

			if (dx * dx + dy * dy < radius * radius)
				callback(&floor->map[ay][ax], ax, ay, context);

			struct tile tile = floor_map_at(floor, ay, ax);
			if (blocked) {
				if (tile_blocks_light(tile)) {
					next_start_slope = tr_slope;
					continue;
				} else { /* No longer blocked. */
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (tile_blocks_light(tile)) {
				blocked = true;
				next_start_slope = tr_slope;
				/* Recurse! */
				raycast_octant_at(floor, x, y, radius, row + 1,
					start_slope, bl_slope, octant, callback,
					context);
			}
		}
		if (blocked) { /* If the last tile of the row blocks light. */
			break;
		}
	}
}

#define SWAP(a, b, type) do { \
	type temp = a; \
	a = b; \
	b = temp; \
} while (0);

/* raycast_octant_at() pretends its always calculating the zeroth octant.
   transform_point_by_octant() applies the transformation. */
static void transform_point_by_octant(int x, int y, int dx, int dy, int octant,
	int *ax, int *ay)
{
	/* The octants are ordered as follows:

		\ 0 | 2 /
		 \  |  /
		1 \ | / 3
		   \|/
		----*----
		   /|\
		5 / | \ 7
		 /  |  \
		/ 4 | 6 \ */
	if (octant & 1) /* 1, 3, 5, 7 */
		SWAP(dx, dy, int);
	if (octant & 2) /* 2, 3, 6, 7 */
		dx = -dx;
	if (octant & 4) /* 4, 5, 6, 7 */
		dy = -dy;

	*ax = x + dx;
	*ay = y + dy;
}
