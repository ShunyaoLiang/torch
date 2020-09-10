use super::World;

use anyhow::anyhow;

use torch_core::color::Color;
use torch_core::frontend::Attributes;
use torch_core::frontend::Screen;
use torch_core::frontend::Size;
use torch_core::shadow::cast as shadow_cast;

pub fn camera(screen: &mut Screen, world: &mut World) {
	let player_pos = world.player_pos();
	let world_clone = world.clone();
	let Size { height, width } = screen.size();
	shadow_cast(world.current_region_mut(), player_pos.into(), 100, |tile, (x, y)| {
		let color = match tile.held_entity {
			Some(key) => world_clone.entity(key)?.color(),
			None => tile.color()
		} * tile.light_level + tile.lighting;
		let token = match tile.held_entity {
			Some(key) => world_clone.entity(key)?.token(),
			None => tile.token(),
		};
		let pos = (
			height.checked_sub(y).ok_or(anyhow!(""))?
				.checked_sub(1).ok_or(anyhow!(""))?,
			x
		);
		if color != Color::BLACK && x < width {
			screen.draw(token, pos, color, Color::BLACK, Attributes::NORMAL);
		}

		Ok(())
	});
}
