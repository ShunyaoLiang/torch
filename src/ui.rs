//! A module that buffers and provides abstractions over creating terminal user interfaces.
//!
//! All terminal interaction is done through the `Ui` struct. There are two primary methods of
//! drawing to the screen: `Ui::render()` which lets the client draw to a buffer which is
//! flushed to the screen at the end, and `ui::Glimmer`s which are scheduled draws to the
//! screen at regular intervals only while the `Ui` is blocking (`Ui::read_event()`).
//!
//! There also exists `Ui::typewrite()`, which draws text to the screen pausing between characters.
//! This is experimental.
//!
//! # Examples
//!
//! ```
//! ui.render(|canvas| {
//! 	canvas.draw("My text!")
//! 		.at(2, 3)
//! 		.fg(Color::from(0xff5000))
//! 		.ink();
//! 	canvas.draw_element(my_element);
//! });
//! ```
//! See `DrawBuilder` for more functions to chain onto `Canvas::draw()`.
//!
//! ```
//! use rand::random;
//!
//! use std::time::Duration;
//!
//! let my_glimmer = Glimmer::new(
//! 	Duration::from_millis(100),
//! 	Box::new(|mut canvas| {
//! 		canvas.draw("Glimmering!")
//! 			.at(4, 4)
//! 			.fg(Color::from(0xa3278f))
//! 			.ink();
//! 		if random::<u8>() % 2 == 0 {
//! 			canvas.lighten(0.2, (4, 14));
//! 		}
//! 	})
//! )
//! ```
//! `Glimmer`s operate on a clone of the buffer, so the drawings of each `Glimmer` are lost on the
//! next draw.
//!
//! **Note:** This entire module uses (line, col).

use crossterm::{self as ct, QueueableCommand};

use std::cmp::{min, Ordering};
use std::io::{stdout, Write};
use std::ops;
use std::slice;
use std::string;
use std::time::{Duration, Instant};

pub use ct::event::Event;
pub use ct::event::KeyCode;

/// Abstracts and buffers terminal interaction.
pub struct Ui {
	lines: u16,
	cols: u16,
	buffer: Buffer,
	pub glimmers: Glimmers,
}

impl Ui {
	pub fn new() -> Self {
		|| -> ct::Result<()> {
			ct::execute!(
				stdout(),
				ct::style::SetAttribute(ct::style::Attribute::Reset),
				ct::cursor::Hide,
				ct::terminal::EnterAlternateScreen,
			)?;
			ct::terminal::enable_raw_mode()?;

			Ok(())
		}().expect("Couldn't set up the terminal");

		let (cols, lines) = ct::terminal::size().unwrap_or_else(|_| {
			eprintln!("Couldn't get the size of the terminal. Defaulting to 24x80");
			(80, 24)
		});

		let buffer = Buffer::new(lines, cols);
		buffer.flush();

		Self { lines, cols, buffer, glimmers: Glimmers::new() }
	}

	pub fn render<F>(&mut self, f: F)
	where
		F: FnOnce(Canvas),
	{
		self.buffer = Buffer::new(self.lines, self.cols);
		f(Canvas::new(&mut self.buffer));
		self.buffer.flush();
	}

	pub fn read_event(&mut self) -> Event {
		let glimmers = &mut self.glimmers;

		loop {
			if glimmers.is_empty() {
				return self.read_event_internal();
			}

			let timeout = glimmers.next_timeout();
			let now = Instant::now();
			match ct::event::poll(timeout) {
				// Interrupted by incoming event.
				Ok(true) => {
					decrement_timeouts(glimmers, min(now.elapsed(), timeout));
					return self.read_event_internal();
				}
				// No event.
				Ok(false) => {
					decrement_timeouts(glimmers, timeout);
					let mut glimmer = glimmers.pop().unwrap();

					self.buffer.resize(self.lines, self.cols);
					let mut buffer = self.buffer.clone();
					(glimmer.view)(Canvas::new(&mut buffer));
					buffer.flush();

					glimmer.timeout = glimmer.interval;
					glimmers.push(glimmer);
				}
				_ => unreachable!(),
			}
		}

		fn decrement_timeouts(glimmers: &mut Glimmers, elapsed: Duration) {
			for glimmer in glimmers {
				glimmer.timeout -= elapsed;
			}
		}
	}

	fn read_event_internal(&mut self) -> Event {
		let event = ct::event::read().unwrap();
		if let Event::Resize(cols, lines) = event {
			self.lines = lines;
			self.cols = cols;
			return self.read_event_internal();
		}

		event
	}

	pub fn size(&self) -> (u16, u16) {
		(self.lines, self.cols)
	}

	#[allow(dead_code)]
	pub fn typewrite<T>(
		&mut self, text: T, pos: (u16, u16), fg: Color, bg: Color, modifiers: Modifiers,
	) where
		T: string::ToString,
	{
		map_text_bounds(
			&text.to_string(),
			pos,
			Rectangle::new((0, 0), (self.lines, self.cols)),
			|c, pos| {
				self.buffer[pos]
					.set_c(c)
					.set_modifiers(modifiers)
					.set_fg(fg)
					.set_bg(bg);
				if !is_event_available() {
					self.buffer.resize(self.lines, self.cols);
					self.buffer.flush();
				}
			},
		);

		if is_event_available() {
			self.buffer.resize(self.lines, self.cols);
			self.buffer.flush();
			self.read_event_internal();
		}

		fn is_event_available() -> bool {
			ct::event::poll(Duration::from_secs(0)).unwrap()
		}
	}
}

impl Drop for Ui {
	fn drop(&mut self) {
		|| -> ct::Result<()> {
			ct::terminal::disable_raw_mode()?;
			ct::execute!(
				stdout(),
				ct::style::SetAttribute(ct::style::Attribute::Reset),
				ct::cursor::Show,
				ct::terminal::LeaveAlternateScreen,
			)?;

			Ok(())
		}().expect("Couldn't clean up the terminal");
	}
}

pub trait Element {
	fn view(&mut self, canvas: &mut Canvas);
}

pub struct Glimmers(Vec<Glimmer>);

impl Glimmers {
	fn new() -> Self {
		Self(Vec::new())
	}

	pub fn push(&mut self, glimmer: Glimmer) {
		match self.0.binary_search(&glimmer) {
			Ok(index) => self.0.insert(index, glimmer),
			Err(index) => self.0.insert(index, glimmer),
		}
	}

	pub fn clear(&mut self) {
		self.0.clear();
	}

	fn is_empty(&self) -> bool {
		self.0.is_empty()
	}

	fn next_timeout(&self) -> Duration {
		self.0.last().unwrap().timeout
	}

	fn pop(&mut self) -> Option<Glimmer> {
		self.0.pop()
	}
}

impl<'a> IntoIterator for &'a mut Glimmers {
	type Item = &'a mut Glimmer;
	type IntoIter = slice::IterMut<'a, Glimmer>;

	fn into_iter(self) -> Self::IntoIter {
		self.0.iter_mut()
	}
}

pub struct Glimmer {
	timeout: Duration,
	interval: Duration,
	view: Box<dyn FnMut(Canvas)>,
}

impl Glimmer {
	pub fn new(interval: Duration, view: Box<dyn FnMut(Canvas)>) -> Self {
		Self { timeout: interval, interval, view }
	}
}

impl Ord for Glimmer {
	fn cmp(&self, other: &Self) -> Ordering {
		self.timeout.cmp(&other.timeout).reverse()
	}
}

impl PartialOrd for Glimmer {
	fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
		Some(self.cmp(other).reverse())
	}
}

impl PartialEq for Glimmer {
	fn eq(&self, other: &Self) -> bool {
		self.timeout == other.timeout
	}
}

impl Eq for Glimmer {}

pub struct Canvas<'buffer> {
	buffer: &'buffer mut Buffer,
}

impl<'buffer> Canvas<'buffer> {
	fn new(buffer: &'buffer mut Buffer) -> Self {
		Self { buffer }
	}

	pub fn draw<T>(&mut self, text: T) -> DrawBuilder
	where
		T: string::ToString,
	{
		DrawBuilder::new(&mut self.buffer, text.to_string())
	}

	pub fn draw_element<E>(&mut self, mut element: E)
	where
		E: Element,
	{
		element.view(self);
	}

	pub fn lighten(&mut self, factor: f32, pos: (u16, u16)) {
		let glyph = &mut self.buffer[pos];
		glyph.fg *= 1f32 + factor;
		glyph.bg *= 1f32 + factor;
	}

	pub fn darken(&mut self, factor: f32, pos: (u16, u16)) {
		self.lighten(-factor, pos);
	}

	pub fn size(&self) -> (u16, u16) {
		(self.buffer.lines, self.buffer.cols)
	}

	pub fn in_bounds(&self, pos: (u16, u16)) -> bool {
		self.buffer.in_bounds(pos)
	}
}

/// Calculates where text would be wrapped within bounds and passes each character with its
/// coordinates to `f`.
fn map_text_bounds<F>(text: &str, (line, col): (u16, u16), bounds: Rectangle, mut f: F)
where
	F: FnMut(char, (u16, u16)),
{
	for (pos, c) in text.chars().enumerate() {
		if c == '\n' {
			map_text_bounds(&text[pos + 1..], (line + 1, col), bounds, f);
			break;
		} else if pos as u16 == bounds.width {
			if line < bounds.height - 1 {
				map_text_bounds(&text[pos..], (line + 1, col), bounds, f);
			}
			break;
		}

		f(c, bounds + (line, col + pos as u16));
	}
}

pub struct DrawBuilder<'buffer> {
	buffer: &'buffer mut Buffer,
	text: String,
	line: u16,
	col: u16,
	bounds: Rectangle,
	modifiers: Modifiers,
	fg: Color,
	bg: Color,
}

impl<'buffer> DrawBuilder<'buffer> {
	fn new(buffer: &'buffer mut Buffer, text: String) -> Self {
		let bounds = Rectangle::new((0, 0), (buffer.lines, buffer.cols));
		Self {
			buffer, text, line: 0, col: 0, bounds,
			fg: Color::default(), bg: Color::default(), modifiers: Modifiers::NONE,
		}
	}

	pub fn ink(self) {
		map_text_bounds(&self.text.clone(), (self.line, self.col), self.bounds, move |c, pos| {
			if self.buffer.in_bounds(pos) {
				self.buffer[pos]
					.set_c(c)
					.set_modifiers(self.modifiers)
					.set_fg(self.fg)
					.set_bg(self.bg);
			}
		});
	}

	pub fn at(mut self, (line, col): (u16, u16)) -> Self {
		self.line = line;
		self.col = col;
		self
	}

	#[allow(dead_code)]
	pub fn max_height(mut self, max_height: u16) -> Self {
		self.bounds.height = max_height;
		self
	}

	pub fn max_width(mut self, max_width: u16) -> Self {
		self.bounds.width = max_width;
		self
	}

	pub fn modifiers(mut self, modifiers: Modifiers) -> Self {
		self.modifiers = modifiers;
		self
	}

	pub fn fg(mut self, fg: Color) -> Self {
		self.fg = fg;
		self
	}

	#[allow(dead_code)]
	pub fn bg(mut self, bg: Color) -> Self {
		self.bg = bg;
		self
	}
}

#[derive(Clone)]
struct Buffer {
	buf: Vec<Glyph>,
	lines: u16,
	cols: u16,
}

impl Buffer {
	fn new(lines: u16, cols: u16) -> Self {
		Self { buf: vec![Glyph::default(); (lines * cols) as usize], lines, cols }
	}

	fn flush(&self) {
		|| -> ct::Result<()> {
			let mut stdout = stdout();
			let first = self.first();
			set_term_attrs(&Glyph::default(), first)?;
			ct::queue!(
				stdout,
				ct::cursor::MoveTo(0, 0),
				ct::style::SetForegroundColor(first.fg.into()),
				ct::style::SetBackgroundColor(first.bg.into()),
				ct::style::Print(first.c),
			)?;

			for pair in self.pairs() {
				set_term_attrs(&pair[0], &pair[1])?;
				stdout.queue(ct::style::Print(pair[1].c))?;
			}
			stdout.flush()?;

			Ok(())
		}().expect("Couldn't flush the UI buffer");

		fn set_term_attrs(from: &Glyph, to: &Glyph) -> ct::Result<()> {
			use ct::style::{Attribute, SetAttribute, SetBackgroundColor, SetForegroundColor};
			let mut stdout = stdout();

			let enable = if !(from.modifiers - to.modifiers).is_empty() {
				ct::queue!(
					stdout,
					SetAttribute(Attribute::Reset),
					SetForegroundColor(to.fg.into()),
					SetBackgroundColor(to.bg.into()),
				)?;

				to.modifiers
			} else {
				if from.fg != to.fg {
					stdout.queue(SetForegroundColor(to.fg.into()))?;
				}
				if from.bg != to.bg {
					stdout.queue(SetBackgroundColor(to.bg.into()))?;
				}

				to.modifiers - from.modifiers
			};

			if enable.contains(Modifiers::BOLD) {
				stdout.queue(SetAttribute(Attribute::Bold))?;
			}
			if enable.contains(Modifiers::UNDERLINE) {
				stdout.queue(SetAttribute(Attribute::Underlined))?;
			}
			if enable.contains(Modifiers::BLINK) {
				stdout.queue(SetAttribute(Attribute::RapidBlink))?;
			}
			if enable.contains(Modifiers::REVERSE) {
				stdout.queue(SetAttribute(Attribute::Reverse))?;
			}

			Ok(())
		}
	}

	fn resize(&mut self, new_lines: u16, new_cols: u16) {
		let mut new_buffer = Buffer::new(new_lines, new_cols);
		for line in 0..new_lines {
			for col in 0..min(self.cols, new_cols) {
				new_buffer[(line, col)] = self[(line, col)];
			}
		}
		*self = new_buffer;
	}

	fn first(&self) -> &Glyph {
		self.buf.first().unwrap()
	}

	fn pairs(&self) -> slice::Windows<Glyph> {
		self.buf.windows(2)
	}

	fn in_bounds(&self, (line, col): (u16, u16)) -> bool {
		line < self.lines && col < self.cols
	}
}

impl ops::Index<(u16, u16)> for Buffer {
	type Output = Glyph;

	fn index(&self, (line, col): (u16, u16)) -> &Self::Output {
		if self.in_bounds((line, col)) {
			&self.buf[(line * self.cols + col) as usize]
		} else {
			panic!(
				"pos out of bounds: size is {:?} but pos is {:?}",
				(self.lines, self.cols),
				(line, col)
			)
		}
	}
}

impl ops::IndexMut<(u16, u16)> for Buffer {
	fn index_mut(&mut self, (line, col): (u16, u16)) -> &mut Self::Output {
		if self.in_bounds((line, col)) {
			&mut self.buf[(line * self.cols + col) as usize]
		} else {
			panic!(
				"pos out of bounds: size is {:?} but pos is {:?}",
				(self.lines, self.cols),
				(line, col)
			)
		}
	}
}

/// A UTF-8 character with attributes, and foreground and background colors.
#[derive(Clone, Copy)]
struct Glyph {
	c: char,
	modifiers: Modifiers,
	fg: Color,
	bg: Color,
}

impl Glyph {
	fn set_c(&mut self, c: char) -> &mut Self {
		self.c = c;
		self
	}

	fn set_modifiers(&mut self, modifiers: Modifiers) -> &mut Self {
		self.modifiers = modifiers;
		self
	}

	fn set_fg(&mut self, fg: Color) -> &mut Self {
		self.fg = fg;
		self
	}

	fn set_bg(&mut self, bg: Color) -> &mut Self {
		self.bg = bg;
		self
	}
}

impl Default for Glyph {
	fn default() -> Self {
		Self { c: ' ', modifiers: Modifiers::NONE, fg: Color::default(), bg: Color::default() }
	}
}

bitflags::bitflags! {
	pub struct Modifiers: u8 {
		const NONE	  = 0;
		const BOLD	  = 1 << 0;
		const UNDERLINE = 1 << 1;
		const BLINK	 = 1 << 2;
		const REVERSE   = 1 << 3;
	}
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Color {
	r: u8,
	g: u8,
	b: u8,
}

impl Color {
	pub fn new(hex: u32) -> Self {
		Self { r: (hex >> 16 & 0xff) as u8, g: (hex >> 8 & 0xff) as u8, b: (hex & 0xff) as u8 }
	}
}

impl Default for Color {
	fn default() -> Self {
		Self::new(0)
	}
}

impl ops::Add for Color {
	type Output = Self;

	fn add(self, other: Self) -> Self {
		Self {
			r: self.r.saturating_add(other.r),
			g: self.g.saturating_add(other.g),
			b: self.b.saturating_add(other.b),
		}
	}
}

impl ops::AddAssign for Color {
	fn add_assign(&mut self, other: Color) {
		self.r = self.r.saturating_add(other.r);
		self.g = self.g.saturating_add(other.g);
		self.b = self.g.saturating_add(other.b);
	}
}

impl ops::Mul<f32> for Color {
	type Output = Self;

	fn mul(self, rhs: f32) -> Self {
		Self {
			r: ((self.r as f32) * rhs) as u8,
			g: ((self.g as f32) * rhs) as u8,
			b: ((self.b as f32) * rhs) as u8,
		}
	}
}

impl ops::MulAssign<f32> for Color {
	fn mul_assign(&mut self, rhs: f32) {
		self.r = (self.r as f32 * rhs) as u8;
		self.g = (self.g as f32 * rhs) as u8;
		self.b = (self.b as f32 * rhs) as u8;
	}
}

impl From<Color> for ct::style::Color {
	fn from(color: Color) -> Self {
		Self::Rgb { r: color.r, g: color.g, b: color.b }
	}
}

#[derive(Clone, Copy)]
pub struct Rectangle {
	/// The line of the top-left corner.
	line: u16,
	/// The column of the top-left corner.
	col: u16,
	/// The height (lines) of the rectangle.
	height: u16,
	/// The width (columns) of the rectangle.
	width: u16,
}

impl Rectangle {
	pub fn new((line, col): (u16, u16), (height, width): (u16, u16)) -> Self {
		Self { line, col, height, width }
	}
}

impl ops::Add<(u16, u16)> for Rectangle {
	type Output = (u16, u16);

	fn add(self, (line, col): (u16, u16)) -> Self::Output {
		(self.line + line, self.col + col)
	}
}
