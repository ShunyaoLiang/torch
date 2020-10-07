use crate::color::Color;
use super::Attributes;
use super::Point;
use super::Result;
use super::Size;

use crossterm::queue;
use crossterm::cursor::MoveTo as MoveCursorTo;
use crossterm::style::Print;
use crossterm::style::SetAttribute;
use crossterm::style::SetAttributes;
use crossterm::style::SetBackgroundColor;
use crossterm::style::SetForegroundColor;

use std::io::Write;
use std::ops::Index;
use std::ops::IndexMut;

#[derive(Clone)]
pub(super) struct Buffer {
	data: Box<[Cell]>,
	size: Size,
}

impl Buffer {
	pub fn new(size: impl Into<Size>) -> Self {
		let size = size.into();
		Self {
			data: vec![Cell::default(); size.area() as usize].into_boxed_slice(),
			size
		}
	}

	pub fn flush(&mut self, writer: &mut impl Write) -> Result<()> {
		queue!(writer, MoveCursorTo(0, 0))?;
		queue_cell(writer, self.first())?;
		for (current, last) in self.data.windows(2).map(|w| (w[1], w[0])) {
			queue_cell_diff(writer, current, last)?;
		}
		writer.flush()?;

		Ok(())
	}

	pub fn size(&self) -> Size {
		self.size
	}

	fn first(&self) -> Cell {
		self.data[0]
	}
}

fn queue_cell(writer: &mut impl Write, cell: Cell) -> Result<()> {
	queue!(
		writer,
		SetForegroundColor(cell.fg_color.into()),
		SetBackgroundColor(cell.bg_color.into()),
		SetAttributes(cell.attributes.into()),
		Print(cell.c),
	)?;

	Ok(())
}

fn queue_cell_diff(writer: &mut impl Write, current: Cell, last: Cell) -> Result<()> {
	if current.fg_color != last.fg_color {
		queue!(writer, SetForegroundColor(current.fg_color.into()))?;
	}
	if current.bg_color != last.bg_color {
		queue!(writer, SetBackgroundColor(current.bg_color.into()))?;
	}
	if current.attributes != last.attributes {
		queue!(
			writer,
			SetAttribute(crossterm::style::Attribute::Reset),	
			SetForegroundColor(current.fg_color.into()),
			SetBackgroundColor(current.bg_color.into()),
			SetAttributes(current.attributes.into()),
		)?;
	}
	queue!(writer, Print(current.c))?;

	Ok(())
}

impl<P> Index<P> for Buffer
where
	P: Into<Point>
{
	type Output = Cell;

	fn index(&self, point: P) -> &Self::Output {
		let point = point.into();
		let Point { line, col } = point;
		if self.size.contains(point) {
			&self.data[(line * self.size.width + col) as usize]
		} else {
			panic!("point out of bounds: point is {} but size is {}", point, self.size);
		}
	}
}

impl<P> IndexMut<P> for Buffer
where
	P: Into<Point>
{
	fn index_mut(&mut self, point: P) -> &mut Self::Output {
		let point = point.into();
		let Point { line, col } = point;
		if self.size.contains(point) {
			&mut self.data[(line * self.size.width + col) as usize]
		} else {
			panic!("point out of bounds: point is {} but size is {}", point, self.size);
		}
	}
}

#[derive(Clone, Copy, Eq, PartialEq)]
pub struct Cell {
	pub c: char,
	pub fg_color: Color,
	pub bg_color: Color,
	pub attributes: Attributes,
}

impl Default for Cell {
	fn default() -> Self {
		Self {
			c: ' ',
			fg_color: Color::BLACK,
			bg_color: Color::BLACK,
			attributes: Attributes::NORMAL
		}
	}
}
