use crate::ai::null_ai;
use crate::ai::snake_ai;
use crate::static_flyweight;
use super::Region;
use super::RegionKey;
use super::point::Point;

use torch_core::color::Color;

use num_rational::Ratio;

#[derive(Debug)]
pub struct Entity {
	pub(super) class: EntityClassId,
	pub(super) pos: Point,
	pub(super) region: RegionKey,
	pub actions: Ratio<u8>,
}

impl Entity {
	pub fn new(class: EntityClassId, pos: impl Into<Point>, region: RegionKey) -> Self {
		Self { class, pos: pos.into(), region, actions: Ratio::new(class.flyweight().speed, 12) }
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

	pub fn occludes(&self) -> bool {
		self.class.flyweight().occludes
	}

	pub fn pos(&self) -> Point {
		self.pos
	}

	pub fn region(&self) -> RegionKey {
		self.region
	}

	pub fn actions_mut(&mut self) -> &mut Ratio<u8> {
		&mut self.actions
	}

	#[inline]
	pub fn ai(&self) -> fn(&Entity, &Region) -> crate::ai::Action {
		self.class.flyweight().ai
	}
}

struct EntityClass {
	token: &'static str,
	color: Color,
	speed: u8,
	occludes: bool,
	ai: fn(&Entity, &Region) -> crate::ai::Action,
}

static_flyweight! {
	#[derive(Debug)]
	pub enum EntityClassId -> EntityClass {
		Player = { token: "@", color: Color::new(0xffffff), speed: 12, occludes: true,  ai: null_ai },
		Torch  = { token: "i", color: Color::new(0xe25822), speed: 0,  occludes: false, ai: null_ai },
		Snake  = { token: "s", color: Color::new(0x47ff2a), speed: 8,  occludes: false, ai: snake_ai },
	}
}

impl std::fmt::Display for EntityClassId {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		std::fmt::Debug::fmt(self, f)
	}
}
