use crate::static_flyweight;
use super::Entity;
use super::Point;
use super::Region;

use torch_core::color::Color;
use torch_core::shadow;

#[derive(Clone, Debug)]
pub struct LightComponent {
	class: LightComponentClassId,
	lit_points: Vec<Point>,
}

impl LightComponent {
	pub fn new(class: LightComponentClassId) -> Self {
		Self { class, lit_points: Vec::new() }
	}

	fn lum(&self) -> f32 {
		self.class.flyweight().lum
	}

	fn color(&self) -> Color {
		self.class.flyweight().color
	}

	pub fn lit_points(&self) -> &Vec<Point> {
		&self.lit_points
	}

	pub fn clear_lit_points(&mut self) {
		self.lit_points.clear();
	}
}

static_flyweight! {
	#[derive(Debug)]
	pub enum LightComponentClassId -> LightComponentClass {
		Player = { lum: 1.8, color: Color::new(0x0a0a0a) },
		Torch  = { lum: 1.,  color: Color::new(0xff5000) },
	}
}

struct LightComponentClass {
	lum: f32,
	color: Color,
}

pub fn cast(light_component: &mut LightComponent, region: &mut Region, entity: &Entity) {
	let pos = entity.pos().into_tuple();
	shadow::cast(region, pos, 8, |region, (x, y)| {
		let tile = &mut region[(x, y)];
		light_component.lit_points.push((x, y).into());
		let distance_2 =
			(pos.0 as f32 - x as f32).powi(2) +
			(pos.1 as f32 - y as f32).powi(2);
		let dlight = light_component.lum() / distance_2.max(1.0);
		tile.light_level += dlight;
		tile.lighting += light_component.color() * dlight;

		Ok(())
	});
}
