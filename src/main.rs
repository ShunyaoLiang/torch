mod world;

use world::camera::camera;
use world::position::Position;
use world::World;

use torch_core::color::Color;
use torch_core::frontend::Attributes;
use torch_core::frontend::Event;
use torch_core::frontend::Frontend;
use torch_core::frontend::KeyCode;
use torch_core::frontend::Size;
use torch_core::shadow::cast as shadow_cast;

use anyhow::anyhow;
use anyhow::Result;

use rand::prelude::*;
use rand::distributions::uniform::Uniform;

use std::io::stdout;
use std::time::Duration;

fn main() -> Result<()> {
	let stdout = stdout();
	let mut frontend = Frontend::new(&stdout)?;

	run(World::new(), frontend)?;

	Ok(())
}

fn run(mut world: World, mut frontend: Frontend) -> Result<()> {
	let mut rng = thread_rng();
	while {
		world.update();
		frontend.render(|mut screen| {
			camera(&mut screen, &mut world);
		})?;

		true
	} {
		let event = frontend.flicker(|mut screen| {
			let player_pos = world.player_pos();
			let Size { height, .. } = screen.size();
			for light_component in world.light_components().values() {
			let shift = match rng.sample(Uniform::from(-1..=1)) {
				-1 => 0.8,
				0 => 1.,
				1 => 1.2,
				_ => unreachable!(),
			};
				for Position { x, y } in &light_component.lit_positions {
					screen.lighten(shift, (height - y - 1, *x));
				}
			}
			
			Some(Duration::from_millis(60))
		})?;
		match event.unwrap() {
			Event::Key(key) => {
				if let KeyCode::Char('q') = key.code {
					break;
				}
				handle(key.code, &mut world, &mut frontend)
			}
			_ => Err(anyhow!("unhandled type of input")),
		};
		/*
		if success.is_err() {
			continue;
		}
		*/
	}

	Ok(())
}

fn handle(key_code: KeyCode, world: &mut World, frontend: &mut Frontend) -> Result<()> {
	match key_code {
		KeyCode::Char('h') => world.offset_player_position((-1, 0))?,
		KeyCode::Char('j') => world.offset_player_position((0, -1))?,
		KeyCode::Char('k') => world.offset_player_position((0, 1))?,
		KeyCode::Char('l') => world.offset_player_position((1, 0))?,
		KeyCode::Char('y') => world.offset_player_position((-1, 1))?,
		KeyCode::Char('u') => world.offset_player_position((1, 1))?,
		KeyCode::Char('b') => world.offset_player_position((-1, -1))?,
		KeyCode::Char('n') => world.offset_player_position((1, -1))?,
		KeyCode::Char('t') => world::place_torch(world, frontend)?,
		_ => Err(anyhow!("unhandled key"))?,
	}

	Ok(())
}
