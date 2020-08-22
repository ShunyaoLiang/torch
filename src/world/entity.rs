use super::components::LightComponent;
use super::position::Offset;
use super::position::Position;
use super::region::Tile;
use super::EntityKey;
use super::Error;
use super::RegionKey;
use super::Result;
use super::World;

use torch_core::color::Color;

#[derive(Clone, Copy, Debug)]
pub struct Entity {
	class: EntityClass,

	region: RegionKey,
	pub position: Position,
}

impl Entity {
	fn new(class: EntityClass, region: RegionKey, position: impl Into<Position>) -> Self {
		Self { class, region, position: position.into() }
	}

	pub fn color(&self) -> Color {
		self.class.color
	}

	pub fn token(&self) -> char {
		self.class.token
	}
}

#[derive(Clone, Copy, Debug)]
pub struct EntityClass {
	token: char,
	color: Color,
	speed: u32,
}

impl EntityClass {
	pub const PLAYER: Self = Self { token: '@', color: Color::new(0xffffff), speed: 12 };
	pub const TORCH: Self = Self { token: 'i', color: Color::new(0xff5000), speed: 0 };
}

pub struct EntityBuilder<'a> {
	world: &'a mut World,
	entity: EntityKey,
	light_component: Option<LightComponent>,
}

impl<'a> EntityBuilder<'a> {
	fn new(world: &'a mut World, entity: EntityKey) -> Self {
		Self { world, entity, light_component: None }
	}

	pub fn light(mut self, light_component: LightComponent) -> Self {
		self.light_component = Some(light_component);

		self
	}

	pub fn create(mut self) -> EntityKey {
		if let Some(light_component) = self.light_component {
			self.world.light_components.insert(self.entity, light_component);
		}

		self.entity
	}
}

impl World {
	pub(super) fn create_entity(
		&mut self, class: EntityClass, region: RegionKey, position: impl Into<Position>
	) -> EntityBuilder {
		let position = position.into();
		let entity = self.entities.insert(Entity::new(class, region, position));
		self.tile_mut(region, position).unwrap().held_entity = Some(entity);
		EntityBuilder::new(self, entity)
	}

	pub fn set_entity_position(
		&mut self, entity: EntityKey, position: impl Into<Position>
	) -> Result<()> {
		let position = position.into();
		let region = self.entity(entity)?.region;
		if self.is_tile_blocked(region, position)? {
			return Err(Error::Movement);
		}

		self.entity_tile_mut(entity)?.held_entity = None;
		self.tile_mut(region, position)?.held_entity = Some(entity);

		self.entity_mut(entity)?.position = position;

		Ok(())
	}

	pub fn offset_entity_position(
		&mut self, entity: EntityKey, offset: impl Into<Offset>
	) -> Result<()> {
		self.set_entity_position(entity, self.entity(entity)?.position.try_add(offset)?)
	}

	pub(super) fn entity(&self, entity: EntityKey) -> Result<&Entity> {
		self.entities.get(entity).ok_or(Error::EntityKey)
	}

	fn entity_mut(&mut self, entity: EntityKey) -> Result<&mut Entity> {
		self.entities.get_mut(entity).ok_or(Error::EntityKey)
	}

	fn entity_tile_mut(&mut self, entity: EntityKey) -> Result<&mut Tile> {
		let Entity { region, position, .. } = self.entity(entity)?;
		self.tile_mut(*region, *position)
	}
}
