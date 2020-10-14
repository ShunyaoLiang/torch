use crate::static_flyweight;
use super::ItemKey;

use torch_core::color::Color;

use std::fmt::Debug;
use std::fmt::Display;
use std::fmt::Formatter;
use std::fmt;

#[derive(Clone, Copy, Debug)]
pub struct Item {
	pub class: ItemClassId,
	pub corroded: bool,
}

impl Item {
	pub fn new(class: ItemClassId) -> Self {
		Self { class, corroded: false }
	}

	pub fn token(&self) -> &'static str {
		self.class.flyweight().token
	}

	pub fn color(&self) -> Color {
		self.class.flyweight().color
	}
}

impl ToString for Item {
	fn to_string(&self) -> String {
		format!("{}{}", if self.corroded { "corroded " } else { "" }, self.class.to_string())
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

impl Display for ItemClassId {
	fn fmt(&self, f: &mut Formatter) -> fmt::Result {
		Debug::fmt(self, f)
	}
}

#[derive(Clone, Debug, Default)]
pub struct InventoryComponent {
	inventory: Vec<ItemKey>,
	// Equipment will probably be here in future.
}

impl InventoryComponent {
	pub fn inventory(&self) -> &Vec<ItemKey> {
		&self.inventory
	}

	pub fn inventory_mut(&mut self) -> &mut Vec<ItemKey> {
		&mut self.inventory
	}
}
