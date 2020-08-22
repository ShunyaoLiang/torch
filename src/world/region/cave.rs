use crate::world::position::Position;

use super::Region;
use super::TileClass;

use rand::prelude::*;

macro_rules! swap_bindings {
    ($a:expr,$b:expr) => {
        let temp = $a;
        $a = $b;
        $b = temp;
    }
}

pub fn new_cave() -> Region {
	let mut region = Region::new();
	let mut rng = thread_rng();
	for tile in &mut *region.tiles {
		if rng.gen_bool(0.45) {
			tile.class = TileClass::CAVE_WALL;
		}
	}

	let mut region_clone = region.clone();

	for _ in 0..12 {
		for x in 0..Region::WIDTH {
			for y in 0..Region::HEIGHT {
				match region[(x, y)].class {
					TileClass::CAVE_WALL => {
						if neighbours(&mut region_clone, (x, y)) < 4 {
							region[(x, y)].class = TileClass::GROUND;
						}
					}
					TileClass::GROUND => {
						if neighbours(&mut region_clone, (x, y)) >= 5 {
							region[(x, y)].class = TileClass::CAVE_WALL;
						}
					}
					_ => (),
				}
			}
		}
		swap_bindings!(region, region_clone);
	}

	region
}

fn neighbours(region: &mut Region, position: impl Into<Position>) -> u8 {
	let Position { x, y } = position.into();
	let x = x as i32;
	let y = y as i32;
	let mut neighbours = 0;
	
	let mut f = |x, y| {
		if x < 0 || y < 0 {
			neighbours += 1;
			return;
		}
		match region.tile((x as u16, y as u16)) {
			Err(_) => neighbours += 1,
			Ok(tile) if tile.class == TileClass::CAVE_WALL => neighbours += 1,
			_ => (),
		}
	};

	f(x - 1, y - 1);
	f(x - 1, y);
	f(x - 1, y + 1);
	f(x, y - 1);
	f(x, y + 1);
	f(x + 1, y - 1);
	f(x + 1, y);
	f(x + 1, y + 1);

	neighbours
}
