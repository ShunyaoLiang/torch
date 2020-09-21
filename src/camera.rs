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
	let pos = world.entity(player).pos().into_tuple();
	let region_clone = &mut world.region(region_key).clone();
	shadow::cast(region_clone, pos, min_cast_radius(), |tile, (x, y)| {
		let (token, mut color) = match tile.held_entity {
			Some(key) =>  {
				let entity = world.entity(key);
				(entity.token(), entity.color())
			}
			None => (tile.token(), tile.color()),
		};
		color = color * tile.light_level + tile.lighting;

		let _ = xy_to_linecol(x, y, screen.size()).map(|point|
			screen.draw(token, point, color, Color::BLACK, Attributes::NORMAL)
		);

		Ok(())
	});
}

fn min_cast_radius() -> u16 {
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
