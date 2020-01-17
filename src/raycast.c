#include "torch.h"

#include <stdbool.h>
#include <math.h>

static void raycast_octant_at(
	const struct raycast_params * params,
	int row,
	float start_slope,
	float end_slope,
	int octant);

static void transform_point_by_octant(
	int y,
	int x,
	int dy,
	int dx,
	int octant,
	int *ay,
	int *ax);

/* Hides hard-coded initial values for raycast_octant_at() */
#define RAYCAST_OCTANT_AT(params, octant) (raycast_octant_at(params, 1, 1.0, 0.0, octant))

/* Uses Björn Bergström's Recursive Shadowcasting Algorithm, which
   divides the map into eight congruent triangular octants, and calculates each
   octant individually. */
void raycast_at(const struct raycast_params * params)
{
	/* raycast_octant_at() does not cast on the origin (y, x). */
	params->callback(&params->floor->map[params->y][params->x], params->y, params->x, params->context);

	for (int octant = 0; octant < 8; ++octant) {
		RAYCAST_OCTANT_AT(params, octant);
	}
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
static void raycast_octant_at(
	const struct raycast_params * params,
	int row,
	float start_slope,
	float end_slope,
	int octant)
{
	if (start_slope < end_slope)
		return;

	/* This function is called recursively on each gap in blocking tiles
	   except for the last one. next_start_slope contains the slope of the
	   top right corner of the rightmost blocking tile. */
	float next_start_slope = start_slope;
	for (; row <= params->radius; ++row) {
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
			int ay, ax;
			transform_point_by_octant(params->y, params->x, dy, dx, octant, &ay, &ax);
			if (!floor_map_in_bounds(ay, ax))
				continue;

			if (dx * dx + dy * dy < params->radius * params->radius)
				params->callback(&params->floor->map[ay][ax], ay, ax, params->context);

			struct tile tile = floor_map_at(params->floor, ay, ax);
			if (blocked) {
				if (tile_blocks_light(tile)) {
					float tr_slope = (dx + 0.5) / (dy - 0.5);
					next_start_slope = tr_slope;
					continue;
				} else { /* Still blocked. */
					blocked = false;
					start_slope = next_start_slope;
				}
			} else if (tile_blocks_light(tile)) {
				blocked = true;
				float tr_slope = (dx + 0.5) / (dy - 0.5);
				next_start_slope = tr_slope;
				float bl_slope = (dx - 0.5) / (dy + 0.5);
				/* Recurse! */
				raycast_octant_at(params, row + 1, start_slope, bl_slope, octant);
			}
		}
		if (blocked) { /* If the last tile of the row blocks light. */
			break;
		}
	}
}

/* raycast_octant_at() pretends its always calculating the zeroth octant.
   transform_point_by_octant() applies the transformation. */
static void transform_point_by_octant(int y, int x, int dy, int dx, int octant,
	int *ay, int *ax)
{
	/* Notice that either yy and xx are non-zero, or yx and xy are non-zero.
	   When yy and xx are zero, yx and xy swap r and x respectively.
	   When yx and xy are zero, y and x flip along the axis respectively. */
	static const int transforms[8][2][2] = {
		/* yy  yx    xx  xy */
		{ { 1,  0}, { 1,  0} },
		{ { 0,  1}, { 0,  1} },
		{ { 0,  1}, { 0, -1} },
		{ { 1,  0}, {-1,  0} },
		{ {-1,  0}, {-1,  0} },
		{ { 0, -1}, { 0, -1} },
		{ { 0, -1}, { 0,  1} },
		{ {-1,  0}, { 1,  0} },
	};

	const int (*t)[2] = transforms[octant];
	*ay = y + dy * t[0][0] + dx * t[0][1];
	*ax = x + dx * t[1][0] + dy * t[1][1];
}
