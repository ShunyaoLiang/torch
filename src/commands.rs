use crate::world::Direction;
use crate::world::EntityClassId;
use crate::world::Entity;
use crate::world::EntityKey;
use crate::world::LightComponent;
use crate::world::LightComponentClassId;
use crate::world::Offset;
use crate::world::Point;
use crate::world::Region;
use crate::world::RegionKey;
use crate::world::TileClassId;
use crate::world::World;

use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;

use anyhow::Result;
use anyhow::anyhow;

pub fn move_player(
	world: &mut World, player: EntityKey, offset: impl Into<Offset>, current_region: &mut RegionKey,
	message_buffer: &mut Vec<String>,
) -> Result<()> {
	let offset = offset.into();
	// Detect if the player is moving off the edge of a region.
	let player_pos = world.entity(player).pos();
	let (yes, direction) = is_player_leaving_region(player_pos, offset);
	if yes {
		let region = world.entity(player).region();
		let new_region = world.region_graph.edges(region).find(|e| *e.2 == direction)
			.ok_or(anyhow!("no connected region in that direction"))?.1;
		world.move_entity_region(player, new_region, direction)?;
		*current_region = new_region;
	} else {
		world.move_entity(player, offset)?;
	}

	let player_pos = world.entities.get(player).unwrap().pos();
	let region = world.regions.get(&*current_region).unwrap();
	if region[player_pos].class == TileClassId::Water {
		for item_key in world.inventory_components.get_mut(player).unwrap().inventory_mut() {
			let item = world.items.get_mut(*item_key).unwrap();
			if !item.corroded {
				message_buffer.push(format!("Your {} corrodes!", item.class.to_string()));
				item.corroded = true;
			}
		}
	}

	Ok(())
}

fn is_player_leaving_region(player_pos: Point, offset: impl Into<Offset>) -> (bool, Direction) {
	let offset = offset.into();
	if player_pos.x == 0 {
		(offset == (-1, 0).into(), Direction::West)
	} else if player_pos.y == 0 {
		(offset == (0, -1).into(), Direction::South)
	} else if player_pos.x == Region::WIDTH - 1 {
		(offset == (1, 0).into(), Direction::East)
	} else if player_pos.y == Region::HEIGHT - 1 {
		(offset == (0, 1).into(), Direction::North)
	} else {
		(false, Direction::South) // Second value should be discarded if first is false.
	}
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
	let player = world.entities.get(player_key).unwrap();
	let pos = player.pos();
	let inventory = world.inventory_components.get_mut(player_key).unwrap();
	let region = world.regions.get_mut(&player.region()).unwrap();
	if let Some(mut items) = region.items_mut().remove(&pos) {
		inventory.inventory_mut().append(&mut items);
	} else {
		return Err(anyhow!("No item(s) to pick up"))
	}

	Ok(())
}
