use crate::static_flyweight;
use super::ItemKey;

use torch_core::color::Color;

#[derive(Clone, Copy, Debug)]
pub struct Item {
	pub class: ItemClassId,
}

impl Item {
	pub fn new(class: ItemClassId) -> Self {
		Self { class }
	}

	pub fn token(self) -> &'static str {
		self.class.flyweight().token
	}

	pub fn color(self) -> Color {
		self.class.flyweight().color
	}
}

pub struct ItemClass {
	token: &'static str,
	color: Color,
}

static_flyweight! {
	#[derive(Debug)]
	pub enum ItemClassId -> ItemClass {
		Sword = { token: "/", color: Color::new(0xffd700) },
	}
}

#[derive(Clone, Debug, Default)]
pub struct InventoryComponent {
	inventory: Vec<ItemKey>,
	// Equipment will probably be here in future.
}

impl InventoryComponent {
	pub fn inventory_mut(&mut self) -> &mut Vec<ItemKey> {
		&mut self.inventory
	}
}
