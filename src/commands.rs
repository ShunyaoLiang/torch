use crate::world::EntityClassId;
use crate::world::Entity;
use crate::world::EntityKey;
use crate::world::LightComponent;
use crate::world::LightComponentClassId;
use crate::world::Offset;
use crate::world::World;

use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;

use anyhow::Result;
use anyhow::anyhow;

// Should later contain interactions such as automatically attacking enemies.
pub fn move_player(world: &mut World, player: EntityKey, offset: impl Into<Offset>) -> Result<()> {
	world.move_entity(player, offset)?;

	Ok(())
}


pub fn place_torch(
	world: &mut World, player: EntityKey, frontend: &mut Frontend
) -> Result<EntityKey> {
	let player_pos = world.entity(player).pos();
	let pos = player_pos + get_direction(frontend).into();
	let torch = world.add_entity(
		Entity::new(EntityClassId::Torch, pos, world.entity(player).region())
	)?;
	world.add_light_component(torch, LightComponent::new(LightComponentClassId::Torch));

	Ok(torch)
}

fn get_direction(frontend: &mut Frontend) -> impl Into<Offset> {
	loop {
		break match frontend.read_event().unwrap() {
			Event::Key(key) => match key.code {
				KeyCode::Char('h') => (-1, 0),
				KeyCode::Char('j') => (0, -1),
				KeyCode::Char('k') => (0, 1),
				KeyCode::Char('l') => (1, 0),
				KeyCode::Char('y') => (-1, 1),
				KeyCode::Char('u') => (1, 1),
				KeyCode::Char('b') => (-1, -1),
				KeyCode::Char('n') => (1, -1),
				_ => continue,
			}
			_ => continue,
		};
	}
}

pub fn pick_up_item(world: &mut World, player_key: EntityKey) -> Result<()> {
	let player = world.entity(player_key);
	let pos = player.pos();
	let mut inventory = (*world.inventory_component(player_key)).clone();
	let region = world.region_mut(player.region());
	if let Some(mut items) = region.items_mut().remove(&pos) {
		inventory.inventory_mut().append(&mut items);
	} else {
		return Err(anyhow!("No item(s) to pick up"))
	}

	*world.inventory_component_mut(player_key) = inventory;

	Ok(())
}
