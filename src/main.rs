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
use world::World;

use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;
use torch_core::frontend::set_status_line;
use torch_core::shadow::cast as shadow_cast;

use anyhow::Result;

use rand::prelude::*;
use rand::distributions::uniform::Uniform;
use rand::rngs::SmallRng;

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

fn run(mut world: World, mut frontend: Frontend, player: EntityKey) -> Result<()> {
	let mut current_region = ("Caves", 0);

	world.update_region_lights(current_region);
	#[allow(unreachable_code)]
	loop {
		frontend.render(|mut screen| {
			camera(&mut screen, &mut world, current_region, player);
		})?;

		let mut visible_tiles = HashSet::new();
		let player_pos = world.entity(player).pos().into_tuple();
		shadow_cast(world.region_mut(current_region), player_pos, camera::min_cast_radius(), |_, pos| {
			visible_tiles.insert(pos);
			Ok(())
		});

		loop {
			match flicker_torches(&world, &mut frontend, &mut visible_tiles)? {
				Event::Key(key) => if match key.code {
					KeyCode::Char('h') => commands::move_player(&mut world, player, (-1, 0)),
					KeyCode::Char('j') => commands::move_player(&mut world, player, (0, -1)),
					KeyCode::Char('k') => commands::move_player(&mut world, player, (0, 1)),
					KeyCode::Char('l') => commands::move_player(&mut world, player, (1, 0)),
					KeyCode::Char('y') => commands::move_player(&mut world, player, (-1, 1)),
					KeyCode::Char('u') => commands::move_player(&mut world, player, (1, 1)),
					KeyCode::Char('b') => commands::move_player(&mut world, player, (-1, -1)),
					KeyCode::Char('n') => commands::move_player(&mut world, player, (1, -1)),
					KeyCode::Char('t') => commands::place_torch(&mut world, player, &mut frontend)
						.map(|_| ()),
					KeyCode::Char('Q') => return Ok(()),
					KeyCode::Char('.') => Ok(()), // no-op
					_ => continue,
				}.is_err() { continue; }
				_ => continue, // 
			}

			*world.entity_mut(player).actions_mut() -= 1;
			break;
		}

		world.update_region(current_region, player);
		world.update_region_lights(current_region);
	}

	Ok(())
}

fn create_player(world: &mut World) -> EntityKey {
	let pos = generate::random_empty_tile(world.region(("Caves", 0)), &mut world.rng_clone());
	let player = world.add_entity(Entity::new(EntityClassId::Player, pos, ("Caves", 0)))
		.unwrap();
	world.add_light_component(player, LightComponent::new(LightComponentClassId::Player));

	player
}

fn flicker_torches(
	world: &World, frontend: &mut Frontend, visible_tiles: &mut HashSet<(u16, u16)>
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
					xy_to_linecol(point.x, point.y, screen.size()).map(|point|
						screen.lighten(shift, point)
					);
				}
			}

		}
		
		Some(Duration::from_millis(80))
	})?.unwrap();

	Ok(event)
}
