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

			let color = tile.seen_color.to_gray();
			if color != Color::BLACK {
				let _ = xy_to_linecol(x, y, screen.size()).map(|point|
					screen.draw(
						tile.seen_token, point, tile.seen_color.to_gray(), Color::BLACK, 
						Attributes::NORMAL
					)
				);
			}
		}
	}
}

fn draw_visible(
	screen: &mut Screen, world: &mut World, region_key: RegionKey, player: EntityKey
) {
	let pos = world.entity(player).pos().into_tuple();
	let entities = &world.entities;
	let items = &world.items;
	let region = world.regions.get_mut(&region_key).unwrap();
	shadow::cast(region, pos, min_cast_radius(), |region, (x, y)| {
		let tile = &region[(x, y)];
		let (token, mut color) = if let Some(key) = tile.held_entity {
			let entity = entities.get(key).unwrap();
			(entity.token(), entity.color())
		} else if let Some(tile_items) = region.items.get(&(x, y).into()) {
			let top_item_key = tile_items.last().unwrap();
			let top_item = items.get(*top_item_key).unwrap();
			(top_item.token(), top_item.color())
		} else {
			(tile.token(), tile.color())
		};
		color = color * tile.light_level + tile.lighting;

		let tile = &mut region[(x, y)];
		tile.seen_token = token;
		tile.seen_color = color;

		if color != Color::BLACK {
			let _ = xy_to_linecol(x, y, screen.size()).map(|point|
				screen.draw(token, point, color, Color::BLACK, Attributes::NORMAL)
			);
		}

		Ok(())
	});
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
