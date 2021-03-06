mod ai;
mod camera;
mod commands;
mod generate;
mod static_flyweight;
mod world;

use camera::camera;
use camera::xy_to_linecol;

use world::Entity;
use world::EntityClassId;
use world::EntityKey;
use world::LightComponent;
use world::LightComponentClassId;
use world::InventoryComponent;
use world::Item;
use world::ItemKey;
use world::Point;
use world::World;

use torch_core::color::Color;
use torch_core::frontend::Attributes;
use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;
use torch_core::frontend::Screen;
use torch_core::frontend::set_status_line;
use torch_core::shadow::cast as shadow_cast;

use anyhow::Result;

use rand::prelude::*;
use rand::distributions::uniform::Uniform;
use rand::rngs::SmallRng;

use slotmap::SlotMap;

use std::collections::HashSet;
use std::io::stdout;
use std::time::Duration;

fn main() -> Result<()> {
	let stdout = stdout();
	let frontend = Frontend::new(&stdout)?;
	set_status_line("Torch");

	let mut world = World::new()
		.create_regions("Caves", generate::Caves, 2);
	let player = create_player(&mut world);

	run(world, frontend, player)?;

	Ok(())
}

#[allow(unreachable_code)]
fn run(mut world: World, mut frontend: Frontend, player: EntityKey) -> Result<()> {
	let mut current_region = ("Caves", 0);

	// Messages from previous rounds.
	let mut message_archive = Vec::new();
	// Messages from the last round.
	let mut message_buffer = Vec::new();

	world.update_region_lights(current_region);
	'game_loop: loop {
		frontend.render(|mut screen| {
			camera(&mut screen, &mut world, current_region, player);
			status(&mut screen, &mut message_buffer);
		})?;
		message_archive.append(&mut message_buffer);

		let mut visible_tiles = HashSet::new();
		let player_pos = world.entity(player).pos().into_tuple();
		shadow_cast(world.region_mut(current_region), player_pos, camera::min_cast_radius(), |region, pos| {
			if region[pos].light_level > 0. {
				visible_tiles.insert(pos);
			}

			Ok(())
		});

		loop {
			match flicker_torches(&world, &mut frontend, &mut visible_tiles, player_pos.into())? {
				Event::Key(key) => if match key.code {
					KeyCode::Char('h') => commands::move_player(&mut world, player, (-1, 0), &mut current_region, &mut message_buffer),
					KeyCode::Char('j') => commands::move_player(&mut world, player, (0, -1), &mut current_region, &mut message_buffer),
					KeyCode::Char('k') => commands::move_player(&mut world, player, (0, 1), &mut current_region, &mut message_buffer),
					KeyCode::Char('l') => commands::move_player(&mut world, player, (1, 0), &mut current_region, &mut message_buffer),
					KeyCode::Char('y') => commands::move_player(&mut world, player, (-1, 1), &mut current_region, &mut message_buffer),
					KeyCode::Char('u') => commands::move_player(&mut world, player, (1, 1), &mut current_region, &mut message_buffer),
					KeyCode::Char('b') => commands::move_player(&mut world, player, (-1, -1), &mut current_region, &mut message_buffer),
					KeyCode::Char('n') => commands::move_player(&mut world, player, (1, -1), &mut current_region, &mut message_buffer),
					KeyCode::Char('t') => commands::place_torch(&mut world, player, &mut frontend)
						.map(|_| ()),
					KeyCode::Char(',') => commands::pick_up_item(&mut world, player),
					KeyCode::Char('i') => {
						inventory(&mut frontend, world.inventory_components.get(player).unwrap(), &world.items);
						continue 'game_loop;
					},
					KeyCode::Char('Q') => return Ok(()),
					KeyCode::Char('.') => Ok(()), // no-op
					_ => continue,
				}.is_err() { continue; }
				_ => continue, // 
			}

			*world.entity_mut(player).actions_mut() -= 1;

			let player_pos = world.entity(player).pos();
			let turn_counter = world.turn_counter;
			world.region_mut(current_region).player_trail.insert(player_pos, turn_counter);

			break;
		}

		world.update_region(current_region, player, &mut message_buffer, &visible_tiles);
		world.update_region_lights(current_region);
	}

	Ok(())
}

fn create_player(world: &mut World) -> EntityKey {
	let region = world.regions.get(&("Caves", 0)).unwrap();
	let pos = generate::random_empty_tile(region, &mut world.rng);
	let player = world.add_entity(Entity::new(EntityClassId::Player, pos, ("Caves", 0)))
		.unwrap();
	world.add_light_component(player, LightComponent::new(LightComponentClassId::Player));
	world.add_inventory_component(player, InventoryComponent::default());

	player
}

fn flicker_torches(
	world: &World, frontend: &mut Frontend, visible_tiles: &mut HashSet<(u16, u16)>,
	player_pos: Point,
) -> Result<Event> {
	let mut rng = SmallRng::from_entropy();
	let range = Uniform::from(-1..=1);
	let event = frontend.flicker(move |screen| {
		for light_component in world.light_components().values() {
			let shift = match rng.sample(range) {
				-1 => 0.8,
				0 => 1.,
				1 => 1.2,
				_ => unreachable!(),
			};

			for point in light_component.lit_points() {
				if visible_tiles.contains(&point.into_tuple()) {
					if let Some(point) = xy_to_linecol(
						point.x, point.y, screen.size(), player_pos
					) {
						screen.lighten(shift, point);
					}
				}
			}

		}
		
		Some(Duration::from_millis(80))
	})?.unwrap();

	Ok(event)
}

fn inventory(
	frontend: &mut Frontend, inventory: &InventoryComponent, items: &SlotMap<ItemKey, Item>
) {
	frontend.render_unclearing(move |mut screen| {
		screen.draw("Inventory", (0, 0), Color::new(0xffffff), Color::BLACK, Attributes::REVERSE);
		for (i, item) in inventory.inventory().iter().enumerate() {
			let item = items.get(*item).unwrap();
			screen.draw(item.token(), (1 + i as u16, 0), item.color(), Color::BLACK, Attributes::NORMAL);
			let s = format!("- {}", item.to_string());
			screen.draw(s, (1 + i as u16, 2), Color::new(0xffffff), Color::BLACK, Attributes::NORMAL);
		}
	}).unwrap();
	let _ = frontend.read_event().unwrap();
}

fn status(screen: &mut Screen, message_buffer: &mut Vec<String>) {
	screen.draw(message_buffer.join(". "), (0, 0), Color::new(0xffffff), Color::BLACK, Attributes::NORMAL);
}
