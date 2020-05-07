use term::terminfo::{parm::Param, parm::Param::Number, TermInfo};

use std::env;
use std::io::stdout;

pub struct Ui {
	buffer: Buffer,
	lowest_dirty_line: Option<usize>,
	default_bg: Rgb,
	terminfo: TermInfo,
}

type Buffer = Vec<Vec<Glyph>>;

impl Ui {
	pub fn new(default_bg: Rgb) -> Self {
		let (lines, cols) = terminal_size();
		let term = env::var("TERM").expect("Failed to detect your terminal. Is TERM set?");
		let mut terminfo = TermInfo::from_name(&term).unwrap_or_else(|_| -> TermInfo {
			eprintln!(
				"Failed to parse the terminfo entry for {}. \
				 Is one installed for your terminal emulator? \
				 Defaulting to xterm.",
				term
			);
			TermInfo::from_name("xterm").expect(
				"Failed to parse the terminfo entry for xterm. \
				 You're on your own.",
			)
		});
		terminfo.strings.insert(
			"setrgbf",
			"\x1b[38;2;%p1%d;%p2%d;%p3%dm".as_bytes().to_vec(),
		);
		terminfo.strings.insert(
			"setrgbb",
			"\x1b[48;2;%p1%d;%p2%d;%p3%dm".as_bytes().to_vec(),
		);
		terminfo
			.apply_cap("smcup", &[], &mut stdout())
			.expect("Failed to enter ca mode.");
		terminfo
			.apply_cap("civis", &[], &mut stdout())
			.expect("Failed to make cursor invisible.");

		Self {
			buffer: empty_buffer(lines, cols, &default_bg),
			lowest_dirty_line: Some(lines - 1),
			default_bg,
			terminfo,
		}
	}

	pub fn render(&mut self, windows: &[Window]) {
		let (lines, cols) = terminal_size();
		self.buffer = empty_buffer(lines, cols, &self.default_bg);

		for window in windows {
			(window.draw)(self, window);
			self.draw_borders(window);
		}

		self.flush_buffer();
	}

	pub fn draw(
		&mut self, window: &Window, mut line: usize, mut col: usize, text: &str, fg: Rgb,
		bg: Option<Rgb>,
	) {
		line += window.line;
		col += window.col;

		for (pos, ch) in text.chars().enumerate() {
			if ch == '\n' {
				line -= window.line;
				col -= window.col;
				self.draw(window, line + 1, col, &text[pos + 1..], fg, bg);
				break;
			}

			if pos == window.cols {
				break;
			}

			self.buffer[line][col + pos] = Glyph {
				ch,
				fg,
				bg: bg.unwrap_or(self.default_bg),
			}
		}

		self.lowest_dirty_line = match self.lowest_dirty_line {
			None => Some(line),
			Some(lowest) => Some(std::cmp::max(line, lowest)),
		}
	}

	fn draw_borders(&mut self, window: &Window) {
		for line in 0..window.lines {
			let (left_ch, right_ch);
			if line == 0 {
				left_ch = "┌";
				right_ch = "┐";
			} else if line == window.lines - 1 {
				left_ch = "└";
				right_ch = "┘";
			} else {
				left_ch = "│";
				right_ch = "│";
			}
			self.draw(window, line, 0, left_ch, Rgb::from(0xcccccc), None);
			self.draw(
				window,
				line,
				window.cols - 1,
				right_ch,
				Rgb::from(0xcccccc),
				None,
			);
		}
		for col in 1..window.cols - 1 {
			self.draw(window, 0, col, "─", Rgb::from(0xcccccc), None);
			self.draw(
				window,
				window.lines - 1,
				col,
				"─",
				Rgb::from(0xcccccc),
				None,
			);
		}
	}

	fn flush_buffer(&mut self) {
		use std::io::Write;

		if self.lowest_dirty_line.is_none() {
			return;
		}

		let bg = &self.buffer[0][0].bg;
		self.terminfo
			.apply_cap("setrgbb", &<[Param; 3]>::from(*bg), &mut stdout())
			.expect("Failed to set background color");
		self.terminfo
			.apply_cap("home", &[], &mut stdout())
			.expect("Failed to home cursor.");

		let mut last = Glyph::new(&self.default_bg);
		for (pos, line) in self.buffer.iter().enumerate() {
			for glyph in line {
				update_term_attrs(&self.terminfo, glyph, &last);
				print!("{}", glyph.ch);
				last = *glyph;
			}

			if pos == self.lowest_dirty_line.unwrap() {
				break;
			}
		}

		stdout().flush().expect("Failed to flush stdout.");
		self.lowest_dirty_line = None;
	}

	pub fn resize(&mut self) {
		let (lines, cols) = terminal_size();
		for line in &mut self.buffer {
			line.resize(cols, Glyph::new(&self.default_bg));
		}
		self.buffer
			.resize(lines, vec![Glyph::new(&self.default_bg); cols]);
	}
}

impl Drop for Ui {
	fn drop(&mut self) {
		self.terminfo
			.apply_cap("cvvis", &[], &mut stdout())
			.expect("Failed to make cursor visible.");
		self.terminfo
			.apply_cap("rmcup", &[], &mut stdout())
			.expect("Failed to exit ca mode.");
	}
}

pub struct Window {
	pub lines: usize,
	pub cols: usize,
	pub line: usize,
	pub col: usize,
	pub draw: Box<dyn Fn(&mut Ui, &Window)>,
}

// Copyright (c) 2016 Ticki under the MIT License
// Stolen from redox-os/termion/src/sys/unix/size.rs
fn terminal_size() -> (usize, usize) {
	use libc::{c_ushort, ioctl, STDOUT_FILENO, TIOCGWINSZ};
	use std::mem::zeroed;

	#[repr(C)]
	struct WinSize {
		row: c_ushort,
		col: c_ushort,
		x: c_ushort,
		y: c_ushort,
	}

	unsafe {
		let mut winsize: WinSize = zeroed();
		match ioctl(STDOUT_FILENO, TIOCGWINSZ.into(), &mut winsize) {
			-1 => panic!("Rekt by POSIX. Failed to get terminal size."),
			_ => (winsize.row as usize, winsize.col as usize),
		}
	}
}

fn empty_buffer(lines: usize, cols: usize, bg: &Rgb) -> Buffer {
	vec![vec![Glyph::new(bg); cols]; lines]
}

fn update_term_attrs(terminfo: &TermInfo, curr: &Glyph, last: &Glyph) {
	if curr.fg != last.fg {
		terminfo
			.apply_cap("setrgbf", &<[Param; 3]>::from(curr.fg), &mut stdout())
			.expect("Failed to set foreground colour");
	}
	if curr.bg != last.bg {
		terminfo
			.apply_cap("setrgbb", &<[Param; 3]>::from(curr.bg), &mut stdout())
			.expect("Failed to set background colour");
	}
}

#[derive(Copy, Clone)]
struct Glyph {
	ch: char,
	fg: Rgb,
	bg: Rgb,
}

impl Glyph {
	fn new(bg: &Rgb) -> Self {
		Self {
			ch: ' ',
			fg: Rgb::from(0),
			bg: Rgb::from(*bg),
		}
	}
}

#[derive(Copy, Clone)]
pub struct Rgb {
	r: u8,
	g: u8,
	b: u8,
}

impl From<u32> for Rgb {
	fn from(hex: u32) -> Self {
		Self {
			r: (hex >> 16 & 0xff) as u8,
			g: (hex >> 8 & 0xff) as u8,
			b: (hex & 0xff) as u8,
		}
	}
}

impl From<Rgb> for [Param; 3] {
	fn from(rgb: Rgb) -> Self {
		[
			Number(rgb.r as i32),
			Number(rgb.g as i32),
			Number(rgb.b as i32),
		]
	}
}

impl PartialEq for Rgb {
	fn eq(&self, other: &Self) -> bool {
		self.r == other.r && self.g == other.g && self.b == other.b
	}
}
