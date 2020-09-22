use crate::static_flyweight;
use super::EntityKey;

use torch_core::color::Color;
use torch_core::shadow;

#[derive(Clone, Debug)]
pub struct Tile {
	pub class: TileClassId,
	pub held_entity: Option<EntityKey>,
	pub held_entity_occludes: bool,
	pub light_level: f32,
	pub lighting: Color,
}

impl Tile {
	pub fn new(class: TileClassId) -> Self {
		Self { class, held_entity: None, held_entity_occludes: false, light_level: 0., lighting: Color::BLACK }
	}

	pub fn token(&self) -> &'static str {
		self.class.flyweight().token
	}

	pub fn color(&self) -> Color {
		self.class.flyweight().color
	}

	pub fn blocks(&self) -> bool {
		self.class.flyweight().blocks || self.held_entity.is_some()
	}
}

impl Default for Tile {
	fn default() -> Self {
		Self::new(TileClassId::Ground)
	}
}

impl shadow::Occluder for Tile {
	fn occludes(&self) -> bool {
		self.class.flyweight().blocks || self.held_entity_occludes
	}
}

struct TileClass {
	token: &'static str,
	color: Color,
	blocks: bool,
}

static_flyweight! {
	#[derive(Debug)]
	pub enum TileClassId -> TileClass {
		Ground = { token: ".", color: Color::new(0x222222), blocks: false },
		CaveWall = { token: "#", color: Color::new(0x222222), blocks: true },
	}
}
