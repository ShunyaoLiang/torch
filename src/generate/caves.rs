use crate::world::Entity;
use crate::world::EntityClassId;
use crate::world::Generator;
use crate::world::Region;
use crate::world::RegionKey;
use crate::world::Tile;
use crate::world::TileClassId;
use crate::world::World;

use super::random_empty_tile;

use rand::Rng;

use std::mem::swap;
use std::ops::Index;
use std::ops::IndexMut;

pub struct Caves;

impl Generator for Caves {
	fn generate_terrain(&mut self, rng: &mut impl Rng) -> Region {
		let mut grid = &mut Grid::new(rng);
		let mut clone = &mut grid.clone();

		for _ in 0..8 {
			iterate(grid, &*clone);
			swap(&mut grid, &mut clone);
		}

		grid.into_region()
	}

	fn generate_entities(&mut self, world: &mut World, key: RegionKey) {
		// XXX: Cloning the Rng object
		let rng = &mut world.rng_clone();
		for _ in 0..10 {
			let pos = random_empty_tile(world.region(key), rng);
			let _ = world.add_entity(Entity::new(EntityClassId::Snake, pos, key));
		}
	}
}

fn iterate(grid: &mut Grid, clone: &Grid) {
	for x in 0..Region::WIDTH {
		for y in 0..Region::HEIGHT {
			let neighbours = clone.neighbours(x, y);
			// Somehow this works.
			grid[(x, y)] = if clone[(x, y)] {
				if neighbours >= 4 {
					true
				} else {
					false
				}
			} else {
				if neighbours >= 5 {
					true
				} else {
					false
				}
			};
		}
	}
}

#[derive(Clone, Debug)]
struct Grid {
	cells: Box<[bool]>,
}

impl Grid {
	fn new(rng: &mut impl Rng) -> Self {
		let mut cells = Vec::with_capacity((Region::HEIGHT * Region::WIDTH) as usize);
		for _ in 0..cells.capacity() {
			cells.push(rng.gen_bool(0.45));
		}

		Self { cells: cells.into_boxed_slice() }
	}

	fn neighbours(&self, x: u16, y: u16) -> u8 {
		let mut neighbours = 0;
		for x_offset in -1..=1 {
			for y_offset in -1..=1 {
				if x_offset == 0 && y_offset == 0 {
					continue;
				}
				neighbours += self.is_alive(x as i32 + x_offset, y as i32 + y_offset) as u8;
			}
		}

		neighbours
	}

	fn is_alive(&self, x: i32, y: i32) -> bool {
		if x >= 0 && x < Region::WIDTH as i32 && y >= 0 && y < Region::HEIGHT as i32 {
			self[(x as u16, y as u16)]
		} else {
			true
		}
	}

	fn into_region(&mut self) -> Region {
		let mut region = Region::new();
		for (cell, tile) in self.cells.iter().zip(region.tiles.iter_mut()) {
			*tile = if *cell {
				Tile::new(TileClassId::CaveWall)
			} else {
				Tile::new(TileClassId::Ground)
			};
		}

		region
	}
}

impl Index<(u16, u16)> for Grid {
	type Output = bool;

	fn index(&self, (x, y): (u16, u16)) -> &Self::Output {
		if x < Region::WIDTH && y < Region::HEIGHT {
			&self.cells[(y * Region::WIDTH + x) as usize]
		} else {
			panic!("out of bounds: pos was {:?}", (x, y));
		}
	}
}

impl IndexMut<(u16, u16)> for Grid {
	fn index_mut(&mut self, (x, y): (u16, u16)) -> &mut Self::Output {
		if x < Region::WIDTH && y < Region::HEIGHT {
			&mut self.cells[(y * Region::WIDTH + x) as usize]
		} else {
			panic!("out of bounds: pos was {:?}", (x, y));
		}
	}
}
