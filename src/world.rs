mod entity;
mod generator;
mod light;
mod inventory;
mod point;
mod region;
mod tile;

use crate::ai::Action;

use num_rational::Ratio;

use petgraph::graphmap::DiGraphMap;

use rand::SeedableRng;
use rand::rngs::SmallRng;

use slotmap::DenseSlotMap;
use slotmap::SecondaryMap;
use slotmap::SlotMap;
use slotmap::new_key_type;

use std::collections::HashMap;
use std::collections::HashSet;
use std::iter::IntoIterator;

pub use entity::Entity;
pub use entity::EntityClassId;

pub use light::LightComponent;
pub use light::LightComponentClassId;

pub use inventory::InventoryComponent;
pub use inventory::Item;
pub use inventory::ItemClassId;

pub use point::Offset;
pub use point::Point;

pub use region::Region;

pub use tile::Tile;
pub use tile::TileClassId;

pub use generator::Generator;

#[derive(Debug)]
pub struct World {
	pub regions: HashMap<RegionKey, Region>,
	pub region_graph: DiGraphMap<RegionKey, Direction>,
	pub entities: DenseSlotMap<EntityKey, Entity>,
	pub light_components: SecondaryMap<EntityKey, LightComponent>,
	pub inventory_components: SecondaryMap<EntityKey, InventoryComponent>,
	pub items: SlotMap<ItemKey, Item>,
	pub rng: SmallRng,
	pub turn_counter: u64,
}

impl World {
	pub fn new() -> Self {
		let rng = if !cfg!(debug_assertions) {
			SmallRng::from_entropy()
		} else {
			SmallRng::seed_from_u64(0)
		};

		Self {
			regions: HashMap::new(),
			region_graph: DiGraphMap::new(),
			entities: DenseSlotMap::with_key(),
			light_components: SecondaryMap::new(),
			inventory_components: SecondaryMap::new(),
			items: SlotMap::with_key(),
			rng,
			turn_counter: 0,
		}
	}

	pub fn create_regions(mut self, ident: &'static str, gen: impl Generator, n: u32) -> Self {
		assert!(!self.regions.contains_key(&(ident, 0)));

		let mut gens = vec![gen; n as usize];

		for n in 0..n {
			let key = (ident, n);
			let gen = &mut gens[n as usize];
			let region = gen.generate_terrain(&mut self.rng);
			self.regions.insert(key, region);
			self.region_graph.add_node(key);
			gen.generate_entities(&mut self, key);
		}

		// Create edges between the generated regions like a linked list.
		for n in 0..n - 1 {
			let (a, b) = ((ident, n), (ident, n + 1));
			self.region_graph.add_edge(a, b, Direction::South);
			self.region_graph.add_edge(b, a, Direction::North);
			gens[n as usize].generate_connection(Direction::South);
			gens[n as usize + 1].generate_connection(Direction::North);
		}

		self
	}

	#[allow(dead_code)]
	pub fn connect_regions<I>(mut self, connections: I) -> Self
	where
		I: IntoIterator<Item = (RegionKey, RegionKey, Direction)>,
	{
		for (a, b, direction) in connections {
			self.region_graph.add_edge(a, b, direction);
			self.region_graph.add_edge(b, a, -direction);
		}

		self
	}

	pub fn update_region(
		&mut self, region_key: RegionKey, player_key: EntityKey, message_buffer: &mut Vec<String>,
		visible_tiles: &HashSet<(u16, u16)>,
	) {
		// Update entities only after the player has used all their actions.
		let player = self.entity_mut(player_key);
		if player.actions.to_integer() >= 1 {
			return;
		}
		// Iterate over every entity until all actions are used.
		loop {
			let mut action_taken = false;
			let mut actions = Vec::new();
			// XXX: We should really be iterating starting from the player, but it should be
			// unnoticeable, pretending the player is the first entity.
			for (key, entity) in self.entities.iter_mut() {
				// Only update entities in the given region.
				if entity.region != region_key && entity.class != EntityClassId::Player {
					continue;
				}
				// Let the entity do something if it has more than one action remaining
				if entity.actions.to_integer() >= 1 {
					let region = self.regions.get(&region_key).unwrap();
					let action = (entity.ai())(entity, region);
					actions.push((key, action));
					entity.actions -= 1;
					action_taken = true;
				}
			}
			// XXX: "AI".
			for (entity, action) in actions {
				match dbg!(action) {
					Action::Move(x, y) => { let _ = self.move_entity(entity, (x, y)); },
					Action::Attack(attackee) => {
						let attacker_s = self.entity(entity).class.to_string();
						let attackee_s = self.entity(attackee).class.to_string();
						if visible_tiles.contains(&self.entity(entity).pos().into_tuple()) {
							message_buffer.push(format!("The {} attacks the {}!", attacker_s, attackee_s))
						}
					}
					_ => (),
				}
			}
			// Check if round should be over; all actions have been taken.
			if !action_taken {
				break;
			}
		}
		
		self.turn_counter += 1;
		self.replenish_entity_actions(region_key);
	}

	fn replenish_entity_actions(&mut self, region_key: RegionKey) {
		for entity in self.entities.values_mut().filter(|e| e.region == region_key) {
			entity.actions += Ratio::new(entity.speed(), 12);
		}
	}

	pub fn update_region_lights(&mut self, region_key: RegionKey) {
		let region = self.regions.get_mut(&region_key)
			.unwrap();
		region.clear_light_info();
		for (entity_key, light_component) in &mut self.light_components {
			let entity = self.entities.get(entity_key)
				.unwrap();
			// Only update the light source entities on the given region.
			if entity.region != region_key {
				continue;
			}

			light_component.clear_lit_points();
			light::cast(light_component, region, entity);
		}
	}

	pub fn region(&self, region_key: RegionKey) -> &Region {
		self.regions.get(&region_key)
			.unwrap()
	}

	pub fn region_mut(&mut self, region_key: RegionKey) -> &mut Region {
		self.regions.get_mut(&region_key)
			.unwrap()
	}

	pub fn add_entity(&mut self, entity: Entity) -> Result<EntityKey> {
		let pos = entity.pos;
		let region = self.regions.get_mut(&entity.region)
			.unwrap_or_else(|| panic!("Entity's region does not exist: {:?}", entity.region));

		if !Region::in_bounds(pos) || region[pos].blocks() {
			return Err(Error::TileBlocks)
		}

		let entity_occludes = entity.occludes();
		let key = self.entities.insert(entity);
		region[pos].held_entity = Some(key);
		region[pos].held_entity_occludes = entity_occludes;

		Ok(key)
	}

	pub fn move_entity(&mut self, entity_key: EntityKey, offset: impl Into<Offset>) -> Result<()> {
		let entity = self.entity(entity_key);
		let region = self.region(entity.region);

		// Make sure there's actually an entity.
		assert!(region[entity.pos].held_entity.is_some());
		assert_eq!(region[entity.pos].held_entity.unwrap(), entity_key);

		let offset = offset.into();
		let pos = self.entity(entity_key).pos + offset;

		if !Region::in_bounds(pos) || region[pos].blocks() {
			return Err(Error::TileBlocks);
		}

		let entity_region = entity.region;
		let entity_pos = entity.pos;
		let entity_occludes = entity.occludes();
		let region = self.region_mut(entity_region);
		region[pos].held_entity = Some(entity_key);
		region[pos].held_entity_occludes = entity_occludes;
		region[entity_pos].held_entity = None;
		region[entity_pos].held_entity_occludes = false;

		self.entity_mut(entity_key).pos = pos;

		Ok(())
	}

	pub fn move_entity_region(
		&mut self, entity_key: EntityKey, region_key: RegionKey, direction: Direction
	) -> Result<()> {
		// TODO: This needs a serious overhaul. Regions should store where valid locations to spawn
		// on the egdes are.
		let entity = self.entities.get_mut(entity_key).unwrap();
		let previous_entity_pos = entity.pos;
		let entity_pos = &mut entity.pos;
		let previous_entity_region = entity.region;
		entity.region = region_key;
		match direction {
			Direction::North => entity_pos.y = 0,
			Direction::South => entity_pos.y = Region::HEIGHT - 1,
			Direction::East => entity_pos.x = 0,
			Direction::West => entity_pos.x = Region::WIDTH - 1,
		}
		let previous_region = self.regions.get_mut(&previous_entity_region).unwrap();
		previous_region[previous_entity_pos].held_entity = None;
		let region = self.regions.get_mut(&region_key).unwrap();
		region[*entity_pos].held_entity = Some(entity_key);

		if region[*entity_pos].blocks() {
			region[*entity_pos].held_entity = None;
			*entity_pos = crate::generate::random_empty_tile(region, &mut self.rng);
			region[*entity_pos].held_entity = Some(entity_key);
		}

		dbg!(&region[*entity_pos]);
		dbg!(entity);

		Ok(())
	}

	pub fn entity(&self, entity_key: EntityKey) -> &Entity {
		self.entities.get(entity_key)
			.expect("Attempted to get a deleted entity")
	}

	pub fn entity_mut(&mut self, entity_key: EntityKey) -> &mut Entity {
		self.entities.get_mut(entity_key)
			.expect("Attempted to get a deleted entity")
	}

	pub fn add_light_component(&mut self, entity_key: EntityKey, light_component: LightComponent) {
		self.light_components.insert(entity_key, light_component);
	}

	pub fn light_components(&self) -> &SecondaryMap<EntityKey, LightComponent> {
		&self.light_components
	}

	// Should macro this maybe.
	pub fn add_inventory_component(
		&mut self, entity_key: EntityKey, inventory_component: InventoryComponent
	) {
		self.inventory_components.insert(entity_key, inventory_component);
	}

	pub fn add_item(&mut self, item: Item, region: RegionKey, pos: impl Into<Point>) -> ItemKey {
		let pos = pos.into();
		let key = self.items.insert(item);
		let region = self.region_mut(region);
		match region.items.get_mut(&pos) {
			Some(items) => items.push(key),
			None => {
				region.items.insert(pos, vec![key]);
			}
		};

		key
	}
}

// TODO: Should probably become a named struct.
pub type RegionKey = (&'static str, u32);

new_key_type! {
	pub struct EntityKey;
}

new_key_type! {
	pub struct ItemKey;
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Direction {
	North,
	South,
	East,
	West,
}

impl std::ops::Neg for Direction {
	type Output = Direction;

	fn neg(self) -> Self::Output {
		match self {
			Self::North => Self::South,
			Self::South => Self::North,
			Self::East => Self::West,
			Self::West => Self::East,
		}
	}
}

#[derive(thiserror::Error, Debug)]
pub enum Error {
	#[error("Attempted to place entity on blocking tile")]
	TileBlocks,
}

pub type Result<T, E = Error> = std::result::Result<T, E>;
