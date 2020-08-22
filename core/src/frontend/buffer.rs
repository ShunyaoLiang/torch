use crate::color::Color;
use super::Attributes;
use super::Point;
use super::Result;
use super::Size;

use crossterm::queue;
use crossterm::cursor::MoveTo as MoveCursorTo;
use crossterm::style::Attribute;
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

	pub fn flush(&mut self, writer: &mut impl Write) -> Result<()>
	{
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
	if colors_equal_and_attributes_cumulate(current, last) {
		let added = current.attributes - last.attributes;
		if added.contains(Attributes::BOLD) {
			queue!(writer, SetAttribute(Attribute::Bold))?
		}
		if added.contains(Attributes::REVERSE) {
			queue!(writer, SetAttribute(Attribute::Reverse))?
		}
		queue!(writer, Print(current.c))?;

		Ok(())
	} else if current == last {
		queue!(writer, Print(current.c))?;

		Ok(())
	} else {
		queue_cell(writer, current)
	}
}

fn colors_equal_and_attributes_cumulate(a: Cell, b: Cell) -> bool {
	a.fg_color == b.fg_color && a.bg_color == b.bg_color && a.attributes > b.attributes
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
