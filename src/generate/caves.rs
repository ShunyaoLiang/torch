use crate::world::Entity;
use crate::world::EntityClassId;
use crate::world::Direction;
use crate::world::Generator;
use crate::world::Item;
use crate::world::ItemClassId;
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

#[derive(Clone)]
pub struct Caves;

impl Generator for Caves {
	fn generate_terrain(&mut self, rng: &mut impl Rng) -> Region {
		let mut grid = &mut Grid::new(rng);
		let mut clone = &mut grid.clone();

		for _ in 0..8 {
			iterate(grid, &*clone);
			swap(&mut grid, &mut clone);
		}

		grid.make_region()
	}

	fn generate_entities(&mut self, world: &mut World, key: RegionKey) {
		// Unfortunately, we have to clone this as we must borrow the entire world object on line
		// 41. This is cheaper than the alternative of collecting all the randomly generated
		// positions, ensuring that they do not overlap, and then adding entities.
		let rng = &mut world.rng.clone();
		for _ in 0..30 {
			let region = world.regions.get(&key).unwrap();
			let pos = random_empty_tile(region, rng);
			let _ = world.add_entity(Entity::new(EntityClassId::Snake, pos, key));
		}
		// Code duplication because we want the earlier region to only have a lifetime to the call
		// to random_empty_tile.
		let region = world.regions.get(&key).unwrap();
		// Pranking the interface. Generating items in generate_entities.
		let pos = random_empty_tile(region, rng);
		world.add_item(Item::new(ItemClassId::Sword), key, pos);
	}

	fn generate_connection(&mut self, _direction: Direction) {}
}

fn iterate(grid: &mut Grid, clone: &Grid) {
	for x in 0..Region::WIDTH {
		for y in 0..Region::HEIGHT {
			let neighbours = clone.neighbours(x, y);
			// Somehow this works.
			grid[(x, y)] = if clone[(x, y)] {
				neighbours >= 4
			} else {
				neighbours >= 5
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

	fn make_region(&mut self) -> Region {
		let mut region = Region::new();
		for (cell, tile) in self.cells.iter().zip(region.tiles.iter_mut()) {
			*tile = if *cell {
				Tile::new(TileClassId::CaveWall)
			} else {
				Tile::new(TileClassId::Ground)
			};
		}

		// DEBUG
		region[(52, 5)] = Tile::new(TileClassId::Water);

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
