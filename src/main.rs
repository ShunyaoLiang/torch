mod ray_cast;
mod ui;
mod world;

use std::cmp::{max, min};
use std::time::Duration;

use ui::{Canvas, Color, Element, Event, Glimmer, KeyCode, Ui};
use world::{Camera, Entity, LightSource, World};

fn main() {
	let world = World::new();
	let ui = Ui::new();
	ignite(world, ui);
}

fn ignite(mut world: World, mut ui: Ui) {
	prelude_screen(&mut ui);

	let mut player = Entity::new((0, 0), '@', Color::new(0xffffff));
	player.light_source = Some(LightSource::new(6, 1f32, Color::new(0xff5000)));

	let mut bokonon = Entity::new((37, 37), '&', Color::new(0x9cdfed));
	bokonon.light_source = Some(LightSource::new(5, 0.4, Color::new(0x9cdfed)));

	let current_place = world.current_place_mut();
	current_place.add_entity(bokonon).unwrap();

	let mut in_debug_mode = true;
	
	'game_loop: loop {
		for entity in &mut current_place.entities {
			entity.cast_light(&mut current_place.map);
		}
		player.cast_light(&mut current_place.map);

		ui.render(|mut canvas| {
			canvas.draw_element(Camera::new(current_place, &player));
			if in_debug_mode {
				canvas.draw_element(Debugger::new(&player));
			}
		});
		current_place.map.clear_lights();

		if let Event::Key(key) = ui.read_event() {
			if match key.code {
				KeyCode::Char('h') => player.move_by(-1, 0, current_place),
				KeyCode::Char('j') => player.move_by(0, -1, current_place),
				KeyCode::Char('k') => player.move_by(0, 1, current_place),
				KeyCode::Char('l') => player.move_by(1, 0, current_place),
				KeyCode::Char('y') => player.move_by(-1, 1, current_place),
				KeyCode::Char('u') => player.move_by(1, 1, current_place),
				KeyCode::Char('b') => player.move_by(-1, -1, current_place),
				KeyCode::Char('n') => player.move_by(1, -1, current_place),
				KeyCode::Char('Q') => break 'game_loop,
				KeyCode::F(3) => {
					in_debug_mode = !in_debug_mode;
					continue 'game_loop;
				},
				_ => Err(()),
			}.is_err() {
				continue 'game_loop;
			}
		}
	}
}

fn prelude_screen(ui: &mut Ui) {
	let (lines, cols) = ui.size();
	let text = "Torch";
	let mut percent = 0;
	let mut is_increasing = true;
	let name_glimmer = Glimmer::new(
		Duration::from_millis(160),
		Box::new(move |mut canvas| {
			canvas.draw(text)
				.at((lines / 2, (cols - text.len() as u16) / 2))
				.fg(Color::new(0xffffff) * (min(percent, 60) as f32 / 60f32))
				.ink();
			if is_increasing && percent < 80 {
				percent += 3;
			} else {
				is_increasing = false;
				if percent > 0 {
					percent = max(percent - 2, 0);
				}
			}
		}),
	);
	ui.glimmers.push(name_glimmer);
	ui.read_event();
	ui.glimmers.clear();
}

struct Debugger<'a> {
	player: &'a Entity,
}

impl<'a> Debugger<'a> {
	fn new(player: &'a Entity) -> Self {
		Self { player }
	}
}

impl Element for Debugger<'_> {
	fn view(&mut self, canvas: &mut Canvas) {
		canvas.draw(format!("player.x: {}\nplayer.y: {}", self.player.x, self.player.y))
			.at((0, 0))
			.fg(Color::new(0x00ff00))
			.ink();
	}
}
