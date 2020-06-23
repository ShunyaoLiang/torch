//! A simple module that abstracts and buffers creating terminal user interfaces.

use crossterm::{
    self as ct,
    ExecutableCommand,
    QueueableCommand,
};

use std::io::stdout;
use std::ops;
use std::slice;

pub use ct::event::Event;

pub struct Ui {
    lines: u16,
    cols: u16,
}

impl Ui {
    pub fn new() -> Self {
        || -> ct::Result<()> {
            stdout()
                .execute(ct::style::SetAttribute(ct::style::Attribute::Reset))?
                .execute(ct::cursor::Hide)?
                .execute(ct::terminal::EnterAlternateScreen)?;
            ct::terminal::enable_raw_mode()?;

            Ok(())
        }().expect("Couldn't set up the terminal");

        let (lines, cols) = terminal_size();

        Self { lines, cols }
    }

    pub fn render<F>(&self, f: F)
    where
        F: FnOnce(u16, u16) -> Box<[Box<dyn Component>]>
    {
        let mut buffer = Buffer::new(self.lines as usize, self.cols as usize);
        for component in f(self.lines, self.cols).iter_mut() {
            let pen = Pen::new(&mut buffer, component.bounds());
            component.view(pen);
        }
        buffer.flush();
    }

    pub fn poll_event(&mut self) -> Event {
        let event = ct::event::read().expect("Couldn't poll for an input event");
        if let Event::Resize(cols, lines) = event {
            self.lines = lines;
            self.cols = cols;
            return self.poll_event();
        }

        event
    }
}

impl Drop for Ui {
    fn drop(&mut self) {
        || -> ct::Result<()> {
            ct::terminal::disable_raw_mode()?;
            stdout()
                .execute(ct::style::SetAttribute(ct::style::Attribute::Reset))?
                .execute(ct::cursor::Show)?
                .execute(ct::terminal::LeaveAlternateScreen)?;
            Ok(())
        }().expect("Couldn't clean up the terminal");
    }
}

pub trait Component {
    fn bounds(&self) -> Rectangle;
    fn view(&mut self, pen: Pen);
}

/// Abstracts drawing to the buffer. Used by `Component`s to draw to buffers.
pub struct Pen<'buf> {
    buffer: &'buf mut Buffer,
    bounds: Rectangle,
}

impl<'buf> Pen<'buf> {
    fn new(buffer: &'buf mut Buffer, bounds: Rectangle) -> Self {
        Self { buffer, bounds }
    }

    pub fn draw_text(
        &mut self, text: &str, (line, col): (u16, u16), fg: Color, bg: Color, modifiers: Modifiers
    ) {
        for (pos, c) in text.chars().enumerate() {
            if c == '\n' {
                self.draw_text(&text[pos + 1..], (line + 1, col), fg, bg, modifiers);
                break;
            } else if col as usize + pos == self.bounds.width {
                // Wrap text if we have the lines to do so.
                if (line as usize) < self.bounds.height - 1 {
                    self.draw_text(&text[pos..], (line + 1, col), fg, bg, modifiers);
                }
                break;
            }
            self.draw(c, (line, col + pos as u16), fg, bg, modifiers);
        }
    }

    pub fn draw(&mut self, c: char, pos: (u16, u16), fg: Color, bg: Color, modifiers: Modifiers) {
        self.buffer[self.bounds + pos]
            .set_c(c)
            .set_modifiers(modifiers)
            .set_fg(fg)
            .set_bg(bg);
    }
}

/// A 2D matrix of glyphs that can be flushed to stdout.
struct Buffer {
    buf: Vec<Glyph>,
    lines: usize,
    cols: usize,
}

impl Buffer {
    fn new(lines: usize, cols: usize) -> Self {
        Self { buf: vec![Glyph::default(); lines * cols], lines, cols, }
    }

    fn flush(&self) {
        use std::io::Write;        

        || -> ct::Result<()> {
            let mut stdout = stdout();
            let first = self.first();
            set_term_attrs(&Glyph::default(), first)?;
            stdout
                .queue(ct::style::SetForegroundColor(first.fg.into()))?
                .queue(ct::style::SetBackgroundColor(first.bg.into()))?
                .queue(ct::cursor::MoveTo(0, 0))?
                .queue(ct::style::Print(first.c))?;

            for pair in self.pairs() {
                set_term_attrs(&pair[0], &pair[1])?;
                stdout.queue(ct::style::Print(pair[1].c))?;
            }
            stdout.flush()?;

            Ok(())
        }().expect("Couldn't flush the UI buffer");
    }

    fn in_bounds(&self, (line, col): (usize, usize)) -> bool {
        line < self.lines && col < self.cols
    }

    fn first(&self) -> &Glyph {
        self.buf.first().unwrap()
    }

    fn pairs(&self) -> slice::Windows<Glyph> {
        self.buf.windows(2)
    }
}

impl ops::Index<(usize, usize)> for Buffer {
    type Output = Glyph;

    fn index(&self, (line, col): (usize, usize)) -> &Self::Output {
        if self.in_bounds((line, col)) {
            &self.buf[line * self.cols + col]
        } else {
            panic!(
                "pos out of bounds: size is {:?} but pos is {:?}",
                (self.lines, self.cols),
                (line, col)
            )
        }
    }
}

impl ops::IndexMut<(usize, usize)> for Buffer {
    fn index_mut(&mut self, (line, col): (usize, usize)) -> &mut Self::Output {
        if self.in_bounds((line, col)) {
            &mut self.buf[line * self.cols + col]
        } else {
            panic!(
                "pos out of bounds: size is {:?} but pos is {:?}",
                (self.lines, self.cols),
                (line, col)
            )
        }
    }
}

fn set_term_attrs(from: &Glyph, to: &Glyph) -> ct::Result<()> {
    use ct::style::{Attribute, SetAttribute, SetBackgroundColor, SetForegroundColor};
    let mut stdout = stdout();

    let enable = if !(from.modifiers - to.modifiers).is_empty() {
        stdout
            .queue(SetAttribute(Attribute::Reset))?
            .queue(SetForegroundColor(to.fg.into()))?
            .queue(SetBackgroundColor(to.bg.into()))?;

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

/// A UTF-8 character with attributes, and foreground and background colors.
#[derive(Clone)]
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
        const NONE      = 0;
        const BOLD      = 1 << 0;
        const UNDERLINE = 1 << 1;
        const BLINK     = 1 << 2;
        const REVERSE   = 1 << 3;
    }
}

#[derive(Clone, Copy, PartialEq)]
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

impl From<Color> for ct::style::Color {
    fn from(color: Color) -> Self {
        Self::Rgb { r: color.r, g: color.g, b: color.b }
    }
}

#[derive(Clone, Copy)]
pub struct Rectangle {
    /// The line of the top-left corner.
    line: usize,
    /// The column of the top-left corner.
    col: usize,
    /// The height (lines) of the rectangle.
    height: usize,
    /// The width (columns) of the rectangle.
    width: usize,
}

impl Rectangle {
    pub fn new((line, col): (usize, usize), (height, width): (usize, usize)) -> Self {
        Self { line, col, height, width }
    }
}

impl ops::Add<(u16, u16)> for Rectangle {
    type Output = (usize, usize);

    fn add(self, (line, col): (u16, u16)) -> Self::Output {
        (self.line + line as usize, self.col + col as usize)
    }
}

/// Returns the current terminal dimensions as (lines, cols).
pub fn terminal_size() -> (u16, u16) {
    let (cols, lines) = ct::terminal::size().map_err(|_| {
        eprintln!("Couldn't get the size of the terminal. Defaulting to 24x80");
        (80, 24)
    }).unwrap();
    (lines, cols)
}
