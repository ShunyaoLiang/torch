use std::ops::Add;
use std::ops::AddAssign;
use std::ops::Mul;
use std::ops::MulAssign;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub struct Color {
	r: u8,
	g: u8,
	b: u8,
}

impl Color {
	pub const fn new(hex: u32) -> Self {
		Self {
			r: (hex >> 16 & 0xff) as u8,
			g: (hex >> 8 & 0xff) as u8,
			b: (hex & 0xff) as u8
		}
	}

	pub const BLACK: Self = Self::new(0);

	pub fn to_gray(self) -> Self {
		let gray = 0.2989 * self.r as f32 + 0.5870 * self.g  as f32 + 0.1140 * self.b as f32;
		Self { r: gray as u8, g: gray as u8, b: gray as u8 }
	}
}

impl Mul<f32> for Color {
	type Output = Self;

	fn mul(self, rhs: f32) -> Self {
		Self {
			r: ((self.r as f32) * rhs) as u8,
			g: ((self.g as f32) * rhs) as u8,
			b: ((self.b as f32) * rhs) as u8,
		}
	}
}

impl MulAssign<f32> for Color {
	fn mul_assign(&mut self, other: f32) {
		*self = *self * other;
	}
}

impl Add for Color {
	type Output = Self;

	fn add(self, rhs: Self) -> Self {
		Self {
			r: self.r.saturating_add(rhs.r),
			g: self.g.saturating_add(rhs.g),
			b: self.b.saturating_add(rhs.b),
		}
	}
}

impl AddAssign for Color {
	fn add_assign(&mut self, other: Self) {
		*self = *self + other;
	}
}

impl From<Color> for crossterm::style::Color {
	fn from(color: Color) -> Self {
		Self::Rgb { r: color.r, g: color.g, b: color.b }
	}
}
