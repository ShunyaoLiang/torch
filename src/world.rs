use crate::ray_cast;
use crate::ui::{Canvas, Color, Element};

use std::cmp::max;
use std::collections::HashSet;
use std::f64::consts::PI;
use std::ops;

pub struct World {
	place: Place,
}

impl World {
	pub fn new() -> Self {
		Self { place: Place::new() }
	}

	pub fn current_place_mut(&mut self) -> &mut Place {
		&mut self.place
	}
}

pub struct Place {
	pub entities: Vec<Entity>,
	pub map: Map,
}

impl Place {
	pub fn new() -> Self {
		Self { entities: Vec::new(), map: Map::default() }
	}

	pub fn add_entity(&mut self, entity: Entity) -> Result<(), ()> {
		let (x, y) = (entity.x, entity.y);
		if !self.map[(x, y)].holds_entity {
			self.entities.push(entity);
			self.map[(x, y)].holds_entity = true;

			Ok(())
		} else {
			Err(())
		}
	}
}

pub struct Map {
	grid: Vec<Tile>,
	width: u16,
	height: u16,
}

impl Map {
	pub fn in_bounds(&self, x: u16, y: u16) -> bool {
		x < self.width && y < self.height
	}

	pub fn clear_lights(&mut self) {
		for tile in &mut self.grid {
			tile.light = 0f32;
			tile.lighting = Color::new(0);
		}
	}
}

impl Default for Map {
	fn default() -> Self {
		Self { grid: vec![Tile::default(); 10_000], width: 100, height: 100 }
	}
}

impl ops::Index<(u16, u16)> for Map {
	type Output = Tile;

	fn index(&self, (x, y): (u16, u16)) -> &Self::Output {
		if self.in_bounds(x, y) {
			&self.grid[(y * self.width + x) as usize]
		} else {
			panic!(
				"pos out of bounds: size is {:?} but pos is {:?}",
				(self.width, self.height),
				(x, y)
			)
		}
	}
}

impl ops::IndexMut<(u16, u16)> for Map {
	fn index_mut(&mut self, (x, y): (u16, u16)) -> &mut Self::Output {
		if self.in_bounds(x, y) {
			&mut self.grid[(y * self.width + x) as usize]
		} else {
			panic!(
				"pos out of bounds: size is {:?} but pos is {:?}",
				(self.width, self.height),
				(x, y)
			)
		}
	}
}

#[derive(Clone, Copy, Debug)]
pub struct Tile {
	holds_entity: bool,
	blocks: bool,
	pub sprite: char,
	pub color: Color,
	light: f32,
	lighting: Color,
}

impl Tile {
	pub fn blocks(&self) -> bool {
		self.blocks || self.holds_entity
	}

	fn color(&self) -> Color {
		self.color * self.light + self.lighting
	}
}

impl Default for Tile {
	fn default() -> Self {
		Self {
			holds_entity: false,
			blocks: false,
			sprite: '.',
			color: Color::new(0x999999),
			light: 0f32,
			lighting: Color::new(0),
		}
	}
}

#[derive(Debug)]
pub struct Entity {
	pub x: u16,
	pub y: u16,
	pub sprite: char,
	pub color: Color,
	pub light_source: Option<LightSource>,
}

impl Entity {
	pub fn new((x, y): (u16, u16), sprite: char, color: Color) -> Self {
		Self { x, y, sprite, color, light_source: None }
	}

	pub fn move_by(&mut self, x: i16, y: i16, place: &mut Place) -> Result<(), ()> {
		let x = (x + self.x as i16) as u16;
		let y = (y + self.y as i16) as u16;

		if place.map.in_bounds(x, y) && !place.map[(x, y)].blocks() {
			place.map[(self.x, self.y)].holds_entity = false;
			place.map[(x, y)].holds_entity = true;
			self.x = x;
			self.y = y;

			Ok(())
		} else {
			Err(())
		}
	}

	pub fn cast_light(&mut self, map: &mut Map) {
		if let Some(light_source) = self.light_source.take() {
			// XXX
			let mut drawn_to = HashSet::new();
			eprint!("8732");
			ray_cast::cast_at(map, self.x, self.y, light_source.radius, |tile, x, y| {
				if drawn_to.contains(&(x, y)) {
					return;
				}

				// TODO: Should have a function for distance between two points.
				let distance = max(
					(((y as i32 - self.y as i32).pow(2) + (x as i32 - self.x as i32).pow(2)) as f64)
						.sqrt() as u16,
					1,
				);
				let dlight = light_source.bright / (distance.pow(1) as f64 * PI) as f32;

				if x == self.x && y == self.y {
					tile.light += light_source.bright;
				} else {
					tile.light += dlight;
					tile.lighting += light_source.color * dlight;
				}

				drawn_to.insert((x, y));
			});
			self.light_source = Some(light_source);
		}
	}
}

pub struct Camera<'a> {
	place: &'a mut Place,
	player: &'a Entity,
}

impl<'a> Camera<'a> {
	pub fn new(place: &'a mut Place, player: &'a Entity) -> Self {
		Self { place, player }
	}

	fn xy_to_linecol(canvas: &mut Canvas, x: u16, y: u16) -> Result<(u16, u16), ()> {
		let (lines, _) = canvas.size();
		let pos = (lines.checked_sub(y + 1).ok_or(())?, x);
		if canvas.in_bounds(pos) {
			Ok(pos)
		} else {
			Err(())
		}
	}
}

impl Element for Camera<'_> {
	fn view(&mut self, canvas: &mut Canvas) {
		ray_cast::cast_at(&mut self.place.map, self.player.x, self.player.y, 100, |tile, x, y| {
			let _ = Self::xy_to_linecol(canvas, x, y).map(|pos| {
				canvas.draw(tile.sprite).at(pos).fg(tile.color()).ink();
			});
		});

		for entity in &self.place.entities {
			let _ = Self::xy_to_linecol(canvas, entity.x, entity.y).map(|pos| {
				canvas.draw(entity.sprite).at(pos).fg(entity.color).ink();
			});
		}

		let _ = Self::xy_to_linecol(canvas, self.player.x, self.player.y).map(|pos|
			canvas.draw(self.player.sprite).at(pos).fg(self.player.color).ink()
		);
	}
}

#[derive(Debug)]
pub struct LightSource {
	pub radius: u16,
	pub bright: f32,
	pub color: Color,
}

impl LightSource {
	pub fn new(radius: u16, bright: f32, color: Color) -> Self {
		Self { radius, bright, color }
	}
}
