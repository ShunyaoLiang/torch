use crate::world::EntityKey;
use crate::world::RegionKey;
use crate::world::Region;
use crate::world::World;

use torch_core::color::Color;
use torch_core::frontend::Attributes;
use torch_core::frontend::Screen;
use torch_core::frontend::Size;
use torch_core::shadow;

use std::cmp::max;

pub fn camera(
	screen: &mut Screen, world: &mut World, region_key: RegionKey, player: EntityKey
) {
	draw_seen(screen, world, region_key);
	draw_visible(screen, world, region_key, player);
}

fn draw_seen(screen: &mut Screen, world: &mut World, region_key: RegionKey) {
	for x in 0..Region::WIDTH {
		for y in 0..Region::HEIGHT {
			let tile = &world.region(region_key)[(x, y)];

			let _ = xy_to_linecol(x, y, screen.size()).map(|point|
				screen.draw(tile.seen_token, point, tile.seen_color.to_gray(), Color::BLACK, Attributes::NORMAL)
			);
		}
	}
}

fn draw_visible(
	screen: &mut Screen, world: &mut World, region_key: RegionKey, player: EntityKey
) {
	let pos = world.entity(player).pos().into_tuple();
	let mut region_clone = world.region(region_key).clone();
	shadow::cast(&mut region_clone, pos, min_cast_radius(), |tile, (x, y)| {
		let region = world.region(region_key);
		let (token, mut color) = if let Some(key) = tile.held_entity {
			let entity = world.entity(key);
			(entity.token(), entity.color())
		} else if let Some(items) = region.items().get(&(x, y).into()) {
			let top_item_key = items.last().unwrap();
			let top_item = world.items().get(*top_item_key).unwrap();
			(top_item.token(), top_item.color())
		} else {
			(tile.token(), tile.color())
		};
		color = color * tile.light_level + tile.lighting;

		tile.seen_token = token;
		tile.seen_color = color;

		let _ = xy_to_linecol(x, y, screen.size()).map(|point|
			screen.draw(token, point, color, Color::BLACK, Attributes::NORMAL)
		);

		Ok(())
	});

	*world.region_mut(region_key) = region_clone;
}

pub fn min_cast_radius() -> u16 {
	max(Region::WIDTH, Region::HEIGHT) / 2
}

pub fn xy_to_linecol(x: u16, y: u16, size: Size) -> Option<(u16, u16)> {
	point(size.height as i32 - y as i32 - 1, x as i32)
}

fn point(line: i32, col: i32) -> Option<(u16, u16)> {
	if line >= 0 && col >= 0 {
		Some((line as u16, col as u16))
	} else {
		None
	}
}
