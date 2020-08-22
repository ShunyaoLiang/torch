use super::EntityKey;
use super::Error;
use super::Position;
use super::RegionKey;
use super::Result;
use super::World;

use torch_core::color::Color;
use torch_core::shadow::Map;
use torch_core::shadow::Occluder;

use std::ops::Index;
use std::ops::IndexMut;

pub mod cave;

#[derive(Clone, Debug)]
pub struct Region {
	pub tiles: Box<[Tile]>,	
}

impl Region {
	const WIDTH: u16 = 100;
	const HEIGHT: u16 = 100;

	pub fn new() -> Self {
		Self {
			tiles: vec![Tile::default(); (Self::WIDTH * Self::HEIGHT) as usize].into_boxed_slice()
		}
	}

	fn tile(&self, position: impl Into<Position>) -> Result<&Tile> {
		let position = position.into();
		if Self::contains(position) {
			Ok(&self[position])
		} else {
			Err(Error::OutOfBounds)
		}
	}

	fn tile_mut(&mut self, position: impl Into<Position>) -> Result<&mut Tile> {
		let position = position.into();
		if Self::contains(position) {
			Ok(&mut self[position])
		} else {
			Err(Error::OutOfBounds)
		}
	}

	fn contains(Position { x, y }: Position) -> bool {
		x < Self::WIDTH && y < Self::HEIGHT
	}
}

impl<P> Index<P> for Region
where
	P: Into<Position>
{
	type Output = Tile;

	fn index(&self, position: P) -> &Self::Output {
		let position = position.into();
		let Position { x, y } = position;
		if Self::contains(position) {
			&self.tiles[(y * Self::WIDTH + x) as usize]
		} else {
			panic!();
		}
	}
}

impl<P> IndexMut<P> for Region
where
	P: Into<Position>
{
	fn index_mut(&mut self, position: P) -> &mut Self::Output {
		let position = position.into();
		let Position { x, y } = position;
		if Self::contains(position) {
			&mut self.tiles[(y * Self::WIDTH + x) as usize]
		} else {
			panic!();
		}
	}
}

impl Map for Region {
	fn in_bounds(&self, x: u16, y: u16) -> bool {
		x < Region::WIDTH && y < Region::HEIGHT
	}
}

#[derive(Clone, Copy, Debug)]
pub struct Tile {
	class: TileClass,
	pub held_entity: Option<EntityKey>,
	pub light_level: f32,
	pub lighting: Color,
}

impl Tile {
	pub fn token(&self) -> char {
		self.class.token
	}

	pub fn color(&self) -> Color {
		self.class.color
	}
}

impl Default for Tile {
	fn default() -> Self {
		Self {
			class: TileClass::default(),
			held_entity: None,
			light_level: 0.,
			lighting: Color::BLACK
		}
	}
}

impl Occluder for Tile {
	fn occludes(&self) -> bool {
		self.class.occludes
	}
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct TileClass {
	token: char,
	color: Color,
	occludes: bool,
}

impl TileClass {
	const GROUND: Self = Self { token: '.', color: Color::new(0x222222), occludes: false };
	const CAVE_WALL: Self = Self { token: '#', color: Color::new(0x222222), occludes: true };
}

impl Default for TileClass {
	fn default() -> Self {
		Self::GROUND
	}
}

impl World {
	pub(super) fn is_tile_blocked(&self, region: RegionKey, position: Position) -> Result<bool> {
		Ok(self.tile(region, position)?.class.occludes)
	}

	pub(super) fn tile(&self, region: RegionKey, position: Position) -> Result<&Tile> {
		self.regions[region].tile(position)
	}

	pub(super) fn tile_mut(&mut self, region: RegionKey, position: Position) -> Result<&mut Tile> {
		self.regions[region].tile_mut(position)
	}
}
