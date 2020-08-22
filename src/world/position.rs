use std::convert::TryFrom;
use std::num::TryFromIntError;
use std::ops::Add;
use std::ops::AddAssign;

#[derive(Clone, Copy, Debug)]
pub struct Position {
	pub x: u16,
	pub y: u16,
}

impl Position {
	pub fn try_add(self, offset: impl Into<Offset>) -> Result<Self, TryFromIntError> {
		let offset = offset.into();
		Ok(Self {
			x: u16::try_from(self.x as i32 + offset.x)?,
			y: u16::try_from(self.y as i32 + offset.y)?,
		})
	}
}

impl From<(u16, u16)> for Position {
	fn from((x, y): (u16, u16)) -> Self {
		Self { x, y }
	}
}

impl From<Position> for (u16, u16) {
	fn from(Position { x, y }: Position) -> Self {
		(x, y)
	}
}

impl<P> Add<P> for Position
where
	P: Into<Position>
{
	type Output = Self;

	fn add(self, rhs: P) -> Self {
		let rhs = rhs.into();
		Self { x: self.x + rhs.x, y: self.y + rhs.y }
	}
}

impl<P> AddAssign<P> for Position
where
	P: Into<Position>
{
	fn add_assign(&mut self, other: P) {
		let other = other.into();
		*self = *self + other;
	}
}

#[derive(Clone, Copy)]
pub struct Offset {
	x: i32,
	y: i32,
}

impl From<(i32, i32)> for Offset {
	fn from((x, y): (i32, i32)) -> Self {
		Self { x, y }
	}
}
