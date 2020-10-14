use crate::world::Entity;
use crate::world::EntityKey;
use crate::world::Point;
use crate::world::Region;

#[derive(Debug)]
pub enum Action {
	Nope, // - Q
	Move(i32, i32),
	Attack(EntityKey),
}

pub fn null_ai(_: &Entity, _: &Region) -> Action {
	Action::Nope
}

macro_rules! continue_if_negative {
	($n:expr) => {
		if $n < 0 {
			continue;
		} else {
			$n
		}
	}
}

pub fn snake_ai(snake: &Entity, region: &Region) -> Action {
	// XXX: One day we'll start using signed integers.
	let snake_pos = snake.pos().into_tuple();
	let (snake_pos_x, snake_pos_y) = (snake_pos.0 as i32, snake_pos.1 as i32);

	let mut min_direction = (0, 0);
	let mut min_light = f32::MAX;
	
	let mut neighbour_light_levels = Vec::new();

	for dx in -1..=1 {
		for dy in -1..=1 {
			let pos: Point = (
				continue_if_negative!(snake_pos_x as i32 + dx) as u16,
				continue_if_negative!(snake_pos_y as i32 + dy) as u16,
			).into();
			let tile = &region[pos];
			let tile_entity = tile.held_entity;
			// Snakes will attack torches and each other, until we dissolve the World object.
			if tile_entity.is_some() && dx != 0 {
				return Action::Attack(tile_entity.unwrap());
			}

			if tile.light_level < min_light {
				min_light = tile.light_level;
				min_direction = (dx, dy);
			}

			neighbour_light_levels.push(tile.light_level);
		}
	}

	let first = neighbour_light_levels[0];
	if neighbour_light_levels.iter().all(|e| *e == first) {
		Action::Move(0, 0)
	} else {
		Action::Move(min_direction.0, min_direction.1)
	}
}
