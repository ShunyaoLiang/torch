use super::point::Point;
use super::tile::Tile;

use torch_core::color::Color;
use torch_core::shadow;

use std::ops::Index;
use std::ops::IndexMut;

#[derive(Clone, Debug)]
pub struct Region {
	pub tiles: Box<[Tile]>,
}

impl Region {
	pub const WIDTH: u16 = 80;
	pub const HEIGHT: u16 = 40;

	pub fn new() -> Self {
		let tiles = vec![Tile::default(); (Self::WIDTH * Self::HEIGHT) as usize]
			.into_boxed_slice();

		Self { tiles }
	}

	pub fn clear_light_info(&mut self) {
		for tile in self.tiles.iter_mut() {
			tile.light_level = 0.;
			tile.lighting = Color::BLACK;
		}
	}
}

impl<I> Index<I> for Region
where
	I: Into<Point>
{
	type Output = Tile;

	fn index(&self, pos: I) -> &Self::Output {
		let Point { x, y } = pos.into();
		if x < Self::WIDTH && y < Self::HEIGHT {
			&self.tiles[(y * Self::WIDTH + x) as usize]
		} else {
			panic!("out of bounds: {:?}", (x, y));
		}
	}
}

impl<I> IndexMut<I> for Region
where
	I: Into<Point>
{
	fn index_mut(&mut self, pos: I) -> &mut Self::Output {
		let Point { x, y } = pos.into();
		if x < Self::WIDTH && y < Self::HEIGHT {
			&mut self.tiles[(y * Self::WIDTH + x) as usize]
		} else {
			panic!("out of bounds: {:?}", (x, y));
		}
	}
}

impl shadow::Map for Region {
	fn in_bounds(&self, x: u16, y: u16) -> bool {
		x < Self::WIDTH && y < Self::HEIGHT
	}
}
