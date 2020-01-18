#include "torch.h"

#include <stdbool.h>
#include <math.h>

struct index_interval
{
	int begin;
	int end;
};

/*
Fetches the selected tile, calls back, and returns
the selected tile.

preconditions:
	- rparams->floor points to a valid floor
	- rparams->callback points to a valid function
	- (x, y) denotes a tile which is in the floor bounds

postconditions:
	- rparams->callback is called with the corresponding tile
	- the result is a pointer to that tile
*/
static struct tile * get_tile_and_call_back(
	const struct raycast_params * rparams,
	int x, int y)
{
	struct tile * tile = &rparams->floor->map[y][x];
	rparams->callback(tile, y, x, rparams->context);
	return tile;
}

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
Transform zeroth octant coordinates into other octants' coordinates.
 
Diagram of octant layout:

	\ 7 | 5 /
	6 \ | / 4
	- - - - -
	2 / | \ 0
	/ 3 | 1 \  
  
If first octant bit is set, flip across the diagonal.
If second octant bit is set, flip across the vertical.
If third octant bit is set, flip across the horizontal.
*/
static struct point transform_point_to_octant(
	int x,
	int y,
	int dx,
	int dy,
	int octant)
{
	if (octant & 1)
	{
		swapi(&dx, &dy);
	}
	if (octant & 2)
	{
		dx = -dx;
	}
	if (octant & 4)
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
	- rparams->floor points to a valid floor
	- rparams->callback is a valid function pointer
	- (dx, dy) denotes a tile which is in the floor bounds

postconditions:
	- rparams->callback is called with the corresponding tile
	- the result is a pointer to that tile
*/
static struct tile * get_transformed_tile_and_call_back(
	const struct raycast_params * rparams,
	int dx, int dy,
	int octant)
{
	struct point r = transform_point_to_octant(
		rparams->x, rparams->y,
		dx, dy,
		octant);
	return get_tile_and_call_back(rparams, r.x, r.y);
}

/*
preconditions:
	- oparams->dx_min <= oparams->dx_max

postconditions:
	- result.begin and result.end denote an in-bounds tile sequence corresponding to the requested slope interval
*/
struct index_interval make_dy_interval(
	float slope_begin,
	float slope_end,
	int dx,
	int dy_max)
{
	struct index_interval dy_interval = {
		slope_begin * (dx - 0.5f) - 0.5f,
		slope_end * (dx + 0.5f) + 0.5f
	};
	dy_interval.begin = min(dy_interval.begin, dy_max);
	dy_interval.end = min(dy_interval.end, dy_max);
	return dy_interval;
}

struct octant_params
{
	int octant;
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

	dx	01234567 dy
		@------- 0				`-` = visible tile
		 ------- 1				`.` = hidden tile
		  ---!.. 2				`#` = blocking tile
		   --#.. 3				`@` = centre
		    --.. 4				`!` = blocking tile where function recurses
			 --. 5
			  -- 6
			   - 7
					 
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
	- rparams->floor points to a valid floor
	- rparams->callback points to a valid function
	- (rparams->x, rparams->y) denotes a tile which is in the floor bounds
	- oparams->dx_min <= oparams->dx_max
	- start_slope < end_slope
	- the coordinates and bounds in rparams and oparams denote a set of tiles
	  which are in the floor bounds

postconditions:
	- rparams->callback is called at least once for each tile in line of sight
	  within the octant and oparams->dx_max 
*/
static void raycast_octant_at(
	const struct raycast_params * rparams,
	const struct octant_params * oparams,
	int dx,
	float start_slope,
	float end_slope)
{
	bool blocked = false;
	for (; dx <= oparams->dx_max && !blocked; ++dx)
	{
		struct index_interval dy_interval = make_dy_interval(
			start_slope,
			end_slope,
			dx,
			oparams->dy_max);

		int dy = dy_interval.begin;		

		struct tile * tile = get_transformed_tile_and_call_back(
			rparams,
			dx, dy,
			oparams->octant);

		blocked = tile_blocks_light(tile);

		for (++dy; dy <= dy_interval.end; ++dy) {
			tile = get_transformed_tile_and_call_back(
				rparams,
				dx, dy,
				oparams->octant);

			if (blocked && !tile_blocks_light(tile)) {
				blocked = false;
				start_slope = (dy - 0.5f) / (dx - 0.5f);
			} else if (!blocked && tile_blocks_light(tile)) {
				blocked = true;
				raycast_octant_at(
					rparams,
					oparams,
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

   Casts on the origin, then enters the recursive functions for each octant.

preconditions:
	- rparams->floor points to a valid floor
	- rparams->callback points to a valid function
	- (rparams->x, rparams->y) denotes a tile which is in the floor bounds
	- the coordinates and bounds in rparams and oparams denote a set of tiles
	  which are in the floor bounds

postconditions:
	- rparams->callback is called at least once for each tile in rparams->radius
	  and line of sight

notes:
	- may call back for cells outside the requested radius.
	- may call back multiple times for cells along axes and diagonals.
*/
void raycast_at(const struct raycast_params * rparams)
{
	get_tile_and_call_back(rparams, rparams->x, rparams->y);

	const int dx_max[] = {
		min(rparams->radius, MAP_COLS - rparams->x - 1),
		min(rparams->radius, MAP_ROWS - rparams->y - 1),
		min(rparams->radius,            rparams->x    ),
		min(rparams->radius, MAP_ROWS - rparams->y - 1),
		min(rparams->radius, MAP_COLS - rparams->x - 1),
		min(rparams->radius,            rparams->y    ),
		min(rparams->radius,            rparams->x    ),
		min(rparams->radius,            rparams->y    ),
	};

	const int dy_max[] = {
		MAP_ROWS - rparams->y - 1,
		MAP_COLS - rparams->x - 1,
		MAP_ROWS - rparams->y - 1,
		           rparams->x    ,
		           rparams->y    ,
		MAP_COLS - rparams->x - 1,
		           rparams->y    ,
		           rparams->x    ,
	};

	for (int octant = 0; octant < 8; octant++)
	{
		raycast_octant_at(
			rparams,
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