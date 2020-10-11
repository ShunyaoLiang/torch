mod buffer;
mod flicker;

use buffer::Buffer;
use buffer::Cell;

use crate::color::Color;

use bitflags::bitflags;

use crossterm::cursor::Hide as HideCursor;
use crossterm::cursor::MoveTo as MoveCursorTo;
use crossterm::cursor::Show as ShowCursor;
use crossterm::event::read as read_event;
use crossterm::execute;
use crossterm::queue;
use crossterm::style::Attribute;
use crossterm::style::SetAttribute;
use crossterm::style::SetBackgroundColor;
use crossterm::terminal::disable_raw_mode;
use crossterm::terminal::enable_raw_mode;
use crossterm::terminal::Clear;
use crossterm::terminal::ClearType;
use crossterm::terminal::EnterAlternateScreen;
use crossterm::terminal::LeaveAlternateScreen;

use std::convert::AsRef;
use std::fmt;
use std::fmt::Display;
use std::fmt::Formatter;
use std::io::Stdout;
use std::io::StdoutLock;
use std::io::Write;

pub use crossterm::event::Event;
pub use crossterm::event::KeyCode;
pub use crossterm::event::KeyModifiers;
pub use crossterm::event::MouseButton;
pub use crossterm::event::MouseEvent;

pub struct Frontend<'a> {
	buffer: Buffer,
	stdout: StdoutLock<'a>,
	size: Size,
}

impl<'a> Frontend<'a> {
	pub fn new(stdout: &'a Stdout) -> Result<Self> {
		let size = terminal_size();
		let mut stdout = stdout.lock();
		execute!(
			stdout,
			HideCursor,
			EnterAlternateScreen,
			MoveCursorTo(0, 0),
			SetBackgroundColor(Color::BLACK.into()),
			Clear(ClearType::All),
		)?;
		enable_raw_mode()?;

		Ok(Self { buffer: Buffer::new(size), stdout, size })
	}

	pub fn render(&mut self, script: impl FnOnce(Screen)) -> Result<()> {
		self.buffer = Buffer::new(self.size);
		self.render_unclearing(script)
	}

	pub fn render_unclearing(&mut self, script: impl FnOnce(Screen)) -> Result<()> {
		script(Screen::new(&mut self.buffer));
		queue!(self.stdout, Clear(ClearType::All))?;
		self.buffer.flush(&mut self.stdout)?;
		execute!(self.stdout, SetAttribute(Attribute::Reset))?;

		Ok(())
	}

	pub fn read_event(&mut self) -> Result<Event> {
		let event = read_event()?;
		if let Event::Resize(cols, lines) = event {
			self.size = (lines, cols).into();
			return self.read_event()
		}

		Ok(event)
	}
}

impl Drop for Frontend<'_> {
	fn drop(&mut self) {
		execute!(self.stdout, ShowCursor, LeaveAlternateScreen).unwrap();
		disable_raw_mode().unwrap();
	}
}

pub struct Screen<'a> {
	buffer: &'a mut Buffer,
}

impl<'a> Screen<'a> {
	fn new(buffer: &'a mut Buffer) -> Self {
		Self { buffer }
	}

	pub fn draw(
		&mut self, text: impl AsRef<str>, point: impl Into<Point>, fg_color: Color, bg_color: Color,
		attributes: Attributes,
	) {
		let point = point.into();
		for (i, c) in text.as_ref().chars().enumerate() {
			let point = point.offset_col(i as i32);
			if !self.size().contains(point) {
				return;
			}
			// Silently fail if we draw outside the terminal.
			self.buffer[point] = Cell { c, fg_color, bg_color, attributes };
		}
	}

	pub fn set_bg_color(&mut self, color: Color) {
		self.buffer.bg_color = color;
	}

	pub fn lighten(&mut self, amount: f32, point: impl Into<Point>) {
		let point = point.into();
		if self.size().contains(point) {
			self.buffer[point].fg_color *= amount;
		}
	}

	pub fn size(&self) -> Size {
		self.buffer.size()
	}
}

#[derive(Clone, Copy)]
pub struct Size {
	pub height: u16,
	pub width: u16,
}

impl Size {
	fn area(&self) -> u16 {
		self.height * self.width
	}

	pub fn contains(&self, point: impl Into<Point>) -> bool {
		let Point { line, col } = point.into();
		line < self.height && col < self.width
	}

	fn index_to_point(&self, index: usize) -> Point {
		(
			(index / self.width as usize) as u16,
			(index % self.width as usize) as u16,
		).into()
	}
}

impl From<(u16, u16)> for Size {
	fn from((height, width): (u16, u16)) -> Self {
		Self { height, width }
	}
}

impl Display for Size {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "({}, {})", self.height, self.width)
	}
}

#[derive(Clone, Copy)]
pub struct Point {
	line: u16,
	col: u16,
}

impl Point {
	pub fn offset_col(self, col: i32) -> Self {
		Self { col: (self.col as i32 + col) as u16, ..self}
	}
}

impl From<(u16, u16)> for Point {
	fn from((line, col): (u16, u16)) -> Self {
		Self { line, col }
	}
}

impl Display for Point {
	fn fmt(&self, f: &mut Formatter<'_>) -> fmt::Result {
		write!(f, "({}, {})", self.line, self.col)
	}
}

bitflags! {
	pub struct Attributes: u8 {
		const NORMAL = 0;
		const BOLD = 1 << 0;
		const REVERSE = 1 << 1;
	}
}

impl From<Attributes> for crossterm::style::Attributes {
	fn from(from: Attributes) -> Self {
		let mut into = crossterm::style::Attributes::default();
		if from.contains(Attributes::BOLD) {
			into.set(crossterm::style::Attribute::Bold);
		}
		if from.contains(Attributes::REVERSE) {
			into.set(crossterm::style::Attribute::Reverse);
		}

		into
	}
}
pub type Result<T> = std::result::Result<T, Error>;

#[derive(thiserror::Error, Debug)]
pub enum Error {
	#[error(transparent)]
	Crossterm(#[from] crossterm::ErrorKind),
	#[error(transparent)]
	Io(#[from] std::io::Error),
}

pub fn set_status_line(status_line: impl AsRef<str>) {
	println!("\x1b]0;{}\x07", status_line.as_ref());
}

fn terminal_size() -> Size {
	let (cols, lines) = crossterm::terminal::size()
		.unwrap();
	Size { height: lines, width: cols }
}
