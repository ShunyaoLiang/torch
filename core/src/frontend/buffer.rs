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
	cells: Box<[Cell]>,
	size: Size,
	pub bg_color: Color,
}

impl Buffer {
	pub fn new(size: impl Into<Size>) -> Self {
		let size = size.into();
		Self {
			cells: vec![Cell::default(); size.area() as usize].into_boxed_slice(),
			size,
			bg_color: Color::BLACK,
		}
	}

	pub fn flush(&self, writer: &mut impl Write) -> Result<()> {
		queue!(writer, MoveCursorTo(0, 0))?;
		queue_cell(writer, self.first())?;

		let mut last_queued_i = 0;
		let mut last_cell = self.first();
		for (i, cell) in self.cells.iter().enumerate().skip(1) {
			// Don't print any blank tiles
			if cell.bg_color == self.bg_color && cell.c == ' ' {
				continue;
			}

			// Is it more efficient to print spaces or a cursor jump?
			let jump = i - last_queued_i;
			if jump > 1 && jump < 8 {
				// Format string hack to print an arbitrary amount of the same character.
				write!(writer, "{: <1$}", "", jump - 1)?;
			} else {
				let Point { line, col } = self.size.index_to_point(i);
				queue!(writer, MoveCursorTo(col, line))?;
			}

			queue_cell_diff(writer, *cell, last_cell)?;
			last_queued_i = i;
			last_cell = *cell;
		}
		writer.flush()?;

		Ok(())
	}

	pub fn size(&self) -> Size {
		self.size
	}

	fn first(&self) -> Cell {
		self.cells[0]
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
			&self.cells[(line * self.size.width + col) as usize]
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
			&mut self.cells[(line * self.size.width + col) as usize]
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
