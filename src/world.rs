mod entity;
mod region;

use components::LightComponent;

use entity::Entity;
use entity::EntityClass;

use region::Region;
use region::cave::new_cave;

use petgraph::graph::DefaultIx;
use petgraph::graph::NodeIndex;
use petgraph::graph::UnGraph;

use rand::prelude::*;

use slotmap::new_key_type;
use slotmap::SecondaryMap;
use slotmap::SlotMap;

use torch_core::color::Color;
use torch_core::shadow::cast as shadow_cast;
use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;

use std::num::TryFromIntError;

pub mod camera;
pub mod position;

pub use position::Offset;
pub use position::Position;

#[derive(Clone, Debug)]
pub struct World {
	entities: SlotMap<EntityKey, Entity>,
	light_components: SecondaryMap<EntityKey, LightComponent>,
	player: EntityKey,
	regions: UnGraph<Region, ()>,
	current_region: RegionKey,
}

impl World {
	pub fn new() -> Self {
		let mut world = Self {
			entities: SlotMap::with_key(),
			light_components: SecondaryMap::new(),
			player: EntityKey::default(),
			regions: UnGraph::new_undirected(),
			current_region: RegionKey::default(),
		};

		let test_region = world.regions.add_node(new_cave());
		world.current_region = test_region;

		let mut pos = (19, 19);
		let mut rng = thread_rng();
		loop {
			world.player = match world.create_entity(EntityClass::PLAYER, test_region, pos)
				.light(LightComponent::PLAYER)
				.create() {
				Err(_) => { pos = (rng.gen_range(0, 100), rng.gen_range(0, 100)); continue; },
				Ok(key) => key,
			};
			break;
		}

		world
	}

	pub fn update(&mut self) {
		self.update_light_components();
	}

	fn update_light_components(&mut self) {
		for tile in self.current_region_mut().tiles.iter_mut() {
			tile.light_level = 0.;
			tile.lighting = Color::BLACK;
		}

		let mut light_components = self.light_components.clone();
		for (key, entity) in self.entities.clone() {
			if !light_components.contains_key(key) {
				continue;
			}
			let mut light_component = light_components.get_mut(key).unwrap();
			light_component.lit_positions.clear();
			let pos = entity.position.into();
			shadow_cast(self.current_region_mut(), pos, 8, |tile, (x, y)| {
				light_component.lit_positions.push((x, y).into());
				let distance_2 =
					(pos.0 as f32 - x as f32).powi(2) +
					(pos.1 as f32 - y as f32).powi(2);
				let dlight = light_component.luminance / distance_2.max(1.0);
				tile.light_level += dlight;
				tile.lighting += light_component.color * dlight;

				Ok(())
			});
		}
		self.light_components = light_components;
	}

	pub fn current_region_mut(&mut self) -> &mut Region {
		self.regions.node_weight_mut(self.current_region).unwrap()
	}

	pub fn player_pos(&self) -> Position {
		self.entity(self.player).unwrap().position
	}

	pub fn offset_player_position(&mut self, offset: impl Into<Offset>) -> Result<()> {
		self.offset_entity_position(self.player, offset)
	}

	pub fn light_components(&self) -> &SecondaryMap<EntityKey, LightComponent> {
		&self.light_components
	}
}

new_key_type! {
	pub struct EntityKey;
}

pub type RegionKey = NodeIndex<DefaultIx>;

#[derive(thiserror::Error, Debug)]
pub enum Error {
	#[error("position already has another entity or blocks")]
	BadPosition,
	#[error("entity does not exist")]
	EntityKey,
	#[error("integer overflow or underflow")]
	Integer(#[from] TryFromIntError),
	#[error("movement illegal")]
	Movement,
	#[error("position out of bounds")]
	OutOfBounds,
	#[error("region does not exist")]
	RegionKey,
}

pub type Result<T> = std::result::Result<T, Error>;

mod components {
	use torch_core::color::Color;

	use super::position::Position;

	#[derive(Clone, Debug)]
	pub struct LightComponent {
		pub luminance: f32,
		pub color: Color,
		pub lit_positions: Vec<Position>,
	}

	impl LightComponent {
		pub const PLAYER: Self = Self::new(1.8, Color::new(0x0a0a0a));
		pub const TORCH: Self = Self::new(1., Color::new(0xff5000));

		const fn new(luminance: f32, color: Color) -> Self {
			Self { luminance, color, lit_positions: Vec::new() }
		}
	}
}

pub fn place_torch(world: &mut World, frontend: &mut Frontend) -> anyhow::Result<()> {
	let pos = world.player_pos().try_add(match frontend.read_event()? {
		Event::Key(key) => {
			match key.code {
				KeyCode::Char('h') => (-1, 0),
				KeyCode::Char('j') => (0, -1),
				KeyCode::Char('k') => (0, 1),
				KeyCode::Char('l') => (1, 0),
				KeyCode::Char('y') => (-1, 1),
				KeyCode::Char('u') => (1, 1),
				KeyCode::Char('b') => (-1, -1),
				KeyCode::Char('n') => (1, -1),
				_ => return place_torch(world, frontend),
			}
		}
		_ => return place_torch(world, frontend),
	})?;

	world.create_entity(EntityClass::TORCH, world.current_region, pos)
		.light(LightComponent::TORCH)
		.create();

	Ok(())
}
