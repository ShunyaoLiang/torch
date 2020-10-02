mod entity;
mod generator;
mod light;
mod inventory;
mod point;
mod region;
mod tile;

use num_rational::Ratio;

use petgraph::graphmap::DiGraphMap;

use rand::Rng;
use rand::SeedableRng;
use rand::rngs::SmallRng;

use slotmap::DenseSlotMap;
use slotmap::SecondaryMap;
use slotmap::SlotMap;
use slotmap::new_key_type;

use std::collections::HashMap;
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
	regions: HashMap<RegionKey, Region>,
	region_graph: DiGraphMap<RegionKey, ()>,
	entities: DenseSlotMap<EntityKey, Entity>,
	light_components: SecondaryMap<EntityKey, LightComponent>,
	inventory_components: SecondaryMap<EntityKey, InventoryComponent>,
	items: SlotMap<ItemKey, Item>,
	rng: SmallRng,
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
		}
	}

	pub fn create_regions(mut self, ident: &'static str, mut gen: impl Generator, n: u32) -> Self {
		assert!(!self.regions.contains_key(&(ident, 0)));

		for n in 0..n {
			let key = (ident, n);
			let region = gen.generate_terrain(&mut self.rng);
			self.regions.insert(key, region);
			self.region_graph.add_node(key);
			gen.generate_entities(&mut self, key);
		}

		// Create edges between the generated regions like a linked list.
		for n in 0..n-1 {
			let (a, b) = ((ident, n), (ident, n + 1));
			self.region_graph.add_edge(a, b, ());
			self.region_graph.add_edge(b, a, ());
		}

		self
	}

	pub fn connect_regions<I>(mut self, connections: I) -> Self
	where
		I: IntoIterator<Item = (RegionKey, RegionKey)>,
	{
		for (a, b) in connections {
			self.region_graph.add_edge(a, b, ());
			self.region_graph.add_edge(b, a, ());
		}

		self
	}

	pub fn update_region(&mut self, region_key: RegionKey, player_key: EntityKey) {
		// Update entities only after the player has used all their actions.
		let player = self.entity_mut(player_key);
		if player.actions.to_integer() >= 1 {
			player.actions -= 1;
			return;
		}
		// Iterate over every entity until all actions are used.
		loop {
			let mut action_was_taken = false;
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
					// TODO: Do an action
					actions.push(key);
					entity.actions -= 1;
					action_was_taken = true;
				}
			}
			// XXX: "AI".
			for entity in actions {
				let _ = self.move_entity(entity, (0, -1));
			}
			// Check if round should be over; all actions have been taken.
			if !action_was_taken {
				self.replenish_entity_actions();
				break;
			}
		}
	}

	fn replenish_entity_actions(&mut self) {
		for entity in self.entities.values_mut() {
			entity.actions += Ratio::new(entity.speed(), 12);
		}
	}

	pub fn update_region_lights(&mut self, region_key: RegionKey) {
		let mut light_components = self.light_components.clone();
		self.region_mut(region_key).clear_light_info();
		for (entity_key, light_component) in &mut light_components {
			let entity = self.entity(entity_key);
			// Only update the light source entities on the given region.
			if entity.region != region_key {
				continue;
			}

			light_component.clear_lit_points();
			light::cast(light_component, self, region_key, entity_key);
		}

		// The clone has the updated lit tile vectors, so we must move it back.
		self.light_components = light_components;
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
			.expect(&format!("Entity's region does not exist: {:?}", entity.region));

		if region[pos].blocks() {
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

		{
			let entity_region = entity.region;
			let entity_pos = entity.pos;
			let entity_occludes = entity.occludes();
			let region = self.region_mut(entity_region);
			region[pos].held_entity = Some(entity_key);
			region[pos].held_entity_occludes = entity_occludes;
			region[entity_pos].held_entity = None;
			region[entity_pos].held_entity_occludes = false;
		}

		self.entity_mut(entity_key).pos = pos;

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

	pub fn inventory_component(&self, key: EntityKey) -> &InventoryComponent {
		self.inventory_components.get(key)
			.unwrap()
	}

	pub fn inventory_component_mut(&mut self, key: EntityKey) -> &mut InventoryComponent {
		self.inventory_components.get_mut(key)
			.unwrap()
	}

	pub fn rng_clone(&self) -> impl Rng {
		self.rng.clone()
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

	pub fn items(&self) -> &SlotMap<ItemKey, Item> {
		&self.items
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

#[derive(thiserror::Error, Debug)]
pub enum Error {
	#[error("Attempted to place entity on blocking tile")]
	TileBlocks,
}

pub type Result<T, E = Error> = std::result::Result<T, E>;
