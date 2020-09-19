use crate::static_flyweight;
use super::RegionKey;
use super::point::Point;

use torch_core::color::Color;

#[derive(Debug)]
pub struct Entity {
	pub class: EntityClassId,
	pub pos: Point,
	pub region: RegionKey,
}

impl Entity {
	pub fn new(class: EntityClassId, pos: impl Into<Point>, region: RegionKey) -> Self {
		Self { class, pos: pos.into(), region }
	}

	pub fn token(&self) -> &'static str {
		self.class.flyweight().token
	}

	pub fn color(&self) -> Color {
		self.class.flyweight().color
	}

	pub fn speed(&self) -> u8 {
		self.class.flyweight().speed
	}
}

struct EntityClass {
	token: &'static str,
	color: Color,
	speed: u8,
}

static_flyweight! {
	#[derive(Debug)]
	pub enum EntityClassId -> EntityClass {
		Player = { token: "@", color: Color::new(0xffffff), speed: 12 },
		Torch  = { token: "i", color: Color::new(0xe25822), speed: 0 },
		Snake  = { token: "s", color: Color::new(0x47ff2a), speed: 8 },
	}
}
