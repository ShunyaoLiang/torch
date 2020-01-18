#include "torch.h"

#include <stdbool.h>
#include <math.h>

/*
Fetches the selected tile, calls back, and returns
the selected tile.

preconditions:
	- raycast->floor points to a valid floor
	- raycast->callback points to a valid function
	- (x, y) denotes a tile which is in the floor bounds

postconditions:
	- raycast->callback is called with the corresponding tile
	- the result is a pointer to that tile
*/
static struct tile * get_tile_and_call_back(
	const struct raycast_params * raycast,
	int x, int y)
{
	struct tile * tile = &raycast->floor->map[y][x];
	raycast->callback(tile, y, x, raycast->context);
	return tile;
}

/*
preconditions:
	- l and r point to valid integers

postconditions:
	- the values of *l and *r are swapped
*/
void swapi(int * l, int * r)
{
	int t = *l;
	*l = *r;
	*r = t;
}

struct point
{
	int x;
	int y;
};

/*
Transforms zeroth octant coordinates into other octants' coordinates.
 
Diagram of octant layout:

	\ 7 | 5 /
	6 \ | / 4
	- - - - -
	2 / | \ 0
	/ 3 | 1 \  
  
If first octant bit is set, flip across the diagonal.
If second octant bit is set, flip across the vertical.
If third octant bit is set, flip across the horizontal.

wide contract
*/
static struct point transform_point_to_octant(
	int x,
	int y,
	int dx,
	int dy,
	int octant_number)
{
	if (octant_number & 1)
	{
		swapi(&dx, &dy);
	}
	if (octant_number & 2)
	{
		dx = -dx;
	}
	if (octant_number & 4)
	{
		dy = -dy;
	}

	return (struct point){ x + dx, y + dy };
}

/*
Transforms the coordinates according to the octant,
fetches the selected tile, calls back, and returns
the selected tile.

preconditions:
	- raycast->floor points to a valid floor
	- raycast->callback is a valid function pointer
	- (dx, dy) denotes a tile which is in the floor bounds

postconditions:
	- raycast->callback is called with the corresponding tile
	- the result is a pointer to that tile
*/
static struct tile * get_transformed_tile_and_call_back(
	const struct raycast_params * raycast,
	int dx, int dy,
	int octant)
{
	struct point r = transform_point_to_octant(
		raycast->x, raycast->y,
		dx, dy,
		octant);
	return get_tile_and_call_back(raycast, r.x, r.y);
}

struct index_interval
{
	int begin;
	int end;
};

/*
Figures the range of y coordinates we need to iterate over for
the given slopes. Clamps this range to fit in floor bounds.

wide contract
*/
struct index_interval make_dy_interval(
	float slope_begin,
	float slope_end,
	int dx,
	int dy_max)
{
	struct index_interval dy_interval = {
		slope_begin * (dx - 0.5f) + 0.5f,
		slope_end * (dx + 0.5f) - 0.5f
	};
	dy_interval.begin = min(dy_interval.begin, dy_max);
	dy_interval.end = min(dy_interval.end, dy_max);
	return dy_interval;
}

struct octant_params
{
	int number;
	int dx_max;
	int dy_max;
};

/*
Accepts slopes corresponding a gap between blocking tiles.
Iterates through columns from left to right, then through each
tile,top to bottom, of the gap, searching for deeper gaps.
Recurses in each gap except those enclosed by the last tile
of the column. Exits if there is no such gap.

Diagram of zeroth octant:

	dx	01234567   dy
				  
		@-------   0				`-` = visible tile
		 -------   1				`.` = hidden tile
		  ---!..   2				`#` = blocking tile
		   --#..   3				`@` = centre
		    --..   4				`!` = blocking tile where function recurses
			 --.   5
			  --   6
			   -   7
					 
The algorithm always works in the coordinates of the zeroth octant,
then applies a transformation to obtain the actual coordinate of the
selected tile.

As a column is iterated:

If the previous tile blocks light, but the current tile does not, we have
encountered the beginning of a gap. If the previous tile allows light, but
the current tile does not, we have encountered the end of the gap, and we recurse.

Each time we encounter any such bound, we calculate the slope of the associated
point on the tile. Slopes for beginnings of regions correspond to bottom left points
of tiles. Slopes for ends of regions correspond to top right points of tiles.
Those slopes are used in next recursion or iteration.

Diagram of begin and end slopes:

	@xxxxxx		       
	    xxxxxxxxx      #######
		   xxx   xxxxxx####### begin
		      xxx      -------
				 xxx   -------
					xxx-------
					   xxx----
					   ---xxx-
					   ------x end
					   #######
					   #######
					   
preconditions:
	- raycast->floor points to a valid floor
	- raycast->callback points to a valid function
	- (raycast->x, raycast->y) denotes a tile which is in the floor bounds
	- octant->dx_min <= octant->dx_max
	- start_slope < end_slope
	- the coordinates and bounds in raycast and octant denote a set of tiles
	  which are in the floor bounds

postconditions:
	- raycast->callback is called at least once for each tile in line of sight
	  within the octant and octant->dx_max 
*/
static void raycast_octant_at(
	const struct raycast_params * raycast,
	const struct octant_params * octant,
	int dx,
	float start_slope,
	float end_slope)
{
	bool blocked = false;
	for (; dx <= octant->dx_max && !blocked; ++dx)
	{
		struct index_interval dy_interval = make_dy_interval(
			start_slope,
			end_slope,
			dx,
			octant->dy_max);

		int dy = dy_interval.begin;		

		struct tile * tile = get_transformed_tile_and_call_back(
			raycast,
			dx, dy,
			octant->number);

		blocked = tile_blocks_light(tile);

		for (++dy; dy <= dy_interval.end; ++dy) {
			tile = get_transformed_tile_and_call_back(
				raycast,
				dx, dy,
				octant->number);

			if (blocked && !tile_blocks_light(tile)) {
				blocked = false;
				start_slope = (dy - 0.5f) / (dx - 0.5f);
			} else if (!blocked && tile_blocks_light(tile)) {
				blocked = true;
				raycast_octant_at(
					raycast,
					octant,
					dx + 1, 
					start_slope,
					(dy - 0.5f) / (dx + 0.5f));
			}
		}
	}
}

/* Uses a modified version ofBjörn Bergström's Recursive Shadowcasting Algorithm,
   which divides the map into eight congruent triangular octants, and calculates
   each octant individually.

   Casts on the origin, then kicks off the recursive functions for each octant.

preconditions:
	- raycast->floor points to a valid floor
	- raycast->callback points to a valid function
	- (raycast->x, raycast->y) denotes a tile which is in the floor bounds
	- the coordinates and bounds in raycast and octant denote a set of tiles
	  which are in the floor bounds

postconditions:
	- raycast->callback is called at least once for each tile in raycast->radius
	  and line of sight

notes:
	- may call back for cells outside the requested radius.
	- may call back multiple times for cells along axes and diagonals.
*/
void raycast_at(const struct raycast_params * raycast)
{
	get_tile_and_call_back(raycast, raycast->x, raycast->y);

	const int dx_max[] = {
		min(raycast->radius, MAP_COLS  - raycast->x - 1),
		min(raycast->radius, MAP_LINES - raycast->y - 1),
		min(raycast->radius,             raycast->x    ),
		min(raycast->radius, MAP_LINES - raycast->y - 1),
		min(raycast->radius, MAP_COLS  - raycast->x - 1),
		min(raycast->radius,             raycast->y    ),
		min(raycast->radius,             raycast->x    ),
		min(raycast->radius,             raycast->y    ),
	};

	const int dy_max[] = {
		MAP_LINES - raycast->y - 1,
		MAP_COLS  - raycast->x - 1,
		MAP_LINES - raycast->y - 1,
		            raycast->x    ,
		            raycast->y    ,
		MAP_COLS  - raycast->x - 1,
		            raycast->y    ,
		            raycast->x    ,
	};

	for (int octant = 0; octant < 8; octant++)
	{
		raycast_octant_at(
			raycast,
			&(struct octant_params) {
				.octant = octant,
				.dx_max = dx_max[octant],
				.dy_max = dy_max[octant]
			},
			1,
			0.0f,
			1.0f);
	}
}