use crate::world::{Map, Tile};

use std::mem;

pub fn cast_at<F>(map: &mut Map, x: u16, y: u16, radius: u16, mut callback: F)
where
	F: FnMut(&mut Tile, u16, u16),
{
	callback(&mut map[(x, y)], x, y);

	for octant in 0..8 {
		cast_octant_at(map, x, y, radius, 1, 1f32, 0f32, octant, &mut callback)
	}

	fn cast_octant_at<F>(
		map: &mut Map, x: u16, y: u16, radius: u16, row: u16, mut start_slope: f32,  end_slope: f32,
		octant: u8, callback: &mut F,
	) where
		F: FnMut(&mut Tile, u16, u16),
	{
		if start_slope < end_slope {
			return;
		}

		let mut next_start_slope = start_slope;
		for row in row..radius + 1 {
			let mut blocked = false;
			for dx in -(row as i16)..1 {
				let dy = -(row as i16);

				let bl_slope = (dx as f32 - 0.5) / (dy as f32 + 0.5);
				let tr_slope = (dx as f32 + 0.5) / (dy as f32 - 0.5);
				if start_slope < tr_slope {
					continue;
				} else if end_slope > bl_slope {
					break;
				}

				let (ax, ay) = transform_point_by_octant(x, y, dx, dy, octant);
				if !map.in_bounds(ax, ay) {
					continue;
				}

				if dx * dx + dy * dy < (radius * radius) as i16 {
					callback(&mut map[(ax, ay)], ax, ay);
				}

				let tile = map[(ax, ay)];
				if blocked {
					if tile.blocks() {
						next_start_slope = tr_slope;
						continue;
					} else {
						blocked = false;
						start_slope = next_start_slope;
					}
				} else if tile.blocks() {
					blocked = true;
					next_start_slope = tr_slope;
					cast_octant_at(
						map, x, y, radius, row + 1, start_slope, bl_slope, octant, callback
					);
				}
			}
			if blocked {
				break;
			}
		}

		fn transform_point_by_octant(
			x: u16, y: u16, mut dx: i16, mut dy: i16, octant: u8,
		) -> (u16, u16) {
			if octant & 1 != 0 {
				mem::swap(&mut dx, &mut dy);
			}
			if octant & 2 != 0 {
				dx = -dx;
			}
			if octant & 4 != 0 {
				dy = -dy;
			}

			((x as i16 + dx) as u16, (y as i16 + dy) as u16)
		}
	}
}
