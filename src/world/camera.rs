use super::World;

use torch_core::color::Color;
use torch_core::frontend::Attributes;
use torch_core::frontend::Screen;
use torch_core::frontend::Size;
use torch_core::shadow::cast as shadow_cast;

pub fn camera(screen: &mut Screen, world: &mut World) {
	let player_pos = world.player_pos();
	let world_clone = world.clone();
	let Size { height, .. } = screen.size();
	shadow_cast(world.current_region_mut(), player_pos.into(), 100, |tile, (x, y)| {
		let color = match tile.held_entity {
			Some(key) => world_clone.entity(key)?.color(),
			None => tile.color()
		} * tile.light_level + tile.lighting;
		let token = match tile.held_entity {
			Some(key) => dbg!(world_clone.entity(key)?.token()),
			None => tile.token(),
		};

		if color != Color::BLACK {
			screen.draw(token, (height - y - 1, x), color, Color::BLACK, Attributes::NORMAL);
		}

		Ok(())
	});
}
