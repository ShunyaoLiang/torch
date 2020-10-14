use std::mem::swap;
use std::ops;

// Sure would be great if this was an iterator.
pub fn cast<M, F>(mut map: &mut M, pos: (u16, u16), radius: u16, mut f: F)
where
	M: Map,
	M::Output: Occluder,
	F: FnMut(&mut M, (u16, u16)) -> anyhow::Result<()>,
{
	let _ = f(&mut map, pos);

	for octant in 0..8 {
		cast_octant(map, pos, radius, 1, 1.0, 0.0, octant, &mut f);
	}
}

fn cast_octant<M, F>(
	map: &mut M, (x, y): (u16, u16), radius: u16, column: u16, mut start_slope: f32,
	end_slope: f32, octant: u8, f: &mut F,
) where
	M: Map,
	M::Output: Occluder,
	F: FnMut(&mut M, (u16, u16)) -> anyhow::Result<()>,
{
	if start_slope < end_slope {
		return;
	}

	let mut next_column_start_slope = start_slope;
	for column in column..=radius {
		let mut last_tile_occludes = false;
		for dy in (0..=column).rev() {
			let dx = column;
			let tl_slope = (dy as f32 + 0.5) / (dx as f32 - 0.5);
			let br_slope = (dy as f32 - 0.5) / (dx as f32 + 0.5);

			if tl_slope < end_slope {
				break;
			} else if br_slope > start_slope {
				continue;
			}

			let (ax, ay) = transform((x, y), (dx, dy), octant);
			if !map.in_bounds(ax, ay) {
				continue;
			}

			if dx.pow(2) + dy.pow(2) <= radius.pow(2) && !should_exclude(dx, dy, octant) {
				let _  = f(map, (ax, ay));
			}

			let current_tile = &map[(ax, ay)];
			if last_tile_occludes {
				if current_tile.occludes() {
					// Keep track of the greatest slope that isn't occluded by a tile.
					next_column_start_slope = br_slope;
					continue;
				}
				// Currently in a gap between occluding tiles.
				last_tile_occludes = false;
				// If there are no more occluding tiles, this loop will break, and this algorithm
				// will begin the next column from this slope. Otherwise, this algorithm will
				// recurse at the next occluding tile, and should begin accordingly.
				start_slope = next_column_start_slope;
			} else if current_tile.occludes() {
				last_tile_occludes = true;
				// Keep track of the greatest slope that isn't occluded by a tile.
				next_column_start_slope = br_slope;
				// Recurse!
				cast_octant(map, (x, y), radius, column + 1, start_slope, tl_slope, octant, f);
			}
		}

		// If the last tile of the column occludes...
		if last_tile_occludes {
			break;
		}
	}
}

fn transform((x, y): (u16, u16), (dx, dy): (u16, u16), octant: u8) -> (u16, u16) {
	let mut dx = dx as i32;
	let mut dy = dy as i32;

	// Mirror across y = +-x.
	if octant & 1 != 0 {
		swap(&mut dx, &mut dy);
	}
	// Mirror over the y-axis.
	if octant & 2 != 0 {
		dx = -dx;
	}
	// Mirror over the x-axis.
	if octant & 4 != 0 {
		dy = -dy;
	}

	(
		(x as i32 + dx) as u16,
		(y as i32 + dy) as u16,
	)
}

fn should_exclude(dx: u16, dy: u16, octant: u8) -> bool {
	let dx = dx as i32;
	let dy = dy as i32;

	// Magic.
	match octant {
		0 | 3 | 5 | 6 => dx == dy,
		1 | 2 | 4 | 7 => dy == 0,
		_ => unreachable!()
	}
}

pub trait Map: ops::IndexMut<(u16, u16)>
where
	Self::Output: Occluder
{
	fn in_bounds(&self, x: u16, y: u16) -> bool;
}

pub trait Occluder {
	fn occludes(&self) -> bool;
}
