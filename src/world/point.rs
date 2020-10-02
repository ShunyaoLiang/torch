use std::cmp::max;
use std::ops::Add;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct Point {
	pub x: u16,
	pub y: u16,
}

impl Point {
	pub fn into_tuple(self) -> (u16, u16) {
		(self.x, self.y)
	}
}

impl From<(u16, u16)> for Point {
	fn from((x, y): (u16, u16)) -> Self {
		Self { x, y }
	}
}

impl Add<Offset> for Point {
	type Output = Self;

	fn add(self, rhs: Offset) -> Self::Output {
		let x = max(self.x as i32 + rhs.x, 0) as u16;
		let y = max(self.y as i32 + rhs.y, 0) as u16;

		Self { x, y }
	}
}

#[derive(Clone, Copy, Debug, Eq, Ord, PartialEq, PartialOrd)]
pub struct Offset {
	pub x: i32,
	pub y: i32,
}

impl From<(i32, i32)> for Offset {
	fn from((x, y): (i32, i32)) -> Self {
		Self { x, y }
	}
}
