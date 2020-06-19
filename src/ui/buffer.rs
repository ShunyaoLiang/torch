use crate::grid::Grid;

use crossterm::{
    cursor::MoveTo,
    style::{Attribute, Color, SetAttribute, SetBackgroundColor, SetForegroundColor},
    QueueableCommand,
};

use std::io;

pub struct Buffer(Grid<Glyph>);

impl Buffer {
    pub fn new(size: (usize, usize)) -> Self {
        Self(Grid::new(size, Glyph::default()))
    }

    pub fn resize(&mut self, size: (usize, usize)) {
        self.0.resize(size, Glyph::default());
    }

    pub fn clear(&mut self) {
        self.0.fill(Glyph::default());
    }

    pub fn draw(
        &mut self, ch: char, (line, col): (u16, u16), fg: Rgb, bg: Rgb, modifiers: Modifiers,
    ) {
        let pos = (line as usize, col as usize);
        if !self.0.in_bounds(pos) {
            return;
        }

        let glyph = &mut self.0[pos];
        glyph.ch = ch;
        glyph.fg = fg;
        glyph.bg = bg;
        glyph.modifiers = modifiers;
    }

    pub fn flush(&self) {
        use io::Write;

        let mut stdout = io::stdout();
        update_term_attrs(&Glyph::default(), self.first());
        stdout.queue(SetForegroundColor(Color::from(self.first().fg))).unwrap();
        stdout.queue(SetBackgroundColor(Color::from(self.first().bg))).unwrap();

        stdout.queue(MoveTo(0, 0)).unwrap();
        for pair in self.windows(2) {
            update_term_attrs(&pair[0], &pair[1]);
            print!("{}", &pair[1].ch);
        }
        stdout.flush().unwrap();
    }

    fn first(&self) -> &Glyph {
        &self.0[(0, 0)]
    }

    fn windows(&self, size: usize) -> std::slice::Windows<Glyph> {
        self.0.windows(size)
    }
}

fn update_term_attrs(from: &Glyph, to: &Glyph) {
    let mut stdout = io::stdout();

    let removed = from.modifiers - to.modifiers;
    let added = if !removed.is_empty() {
        stdout.queue(SetAttribute(Attribute::Reset)).unwrap();
        stdout.queue(SetForegroundColor(Color::from(to.fg))).unwrap();
        stdout.queue(SetBackgroundColor(Color::from(to.bg))).unwrap();

        to.modifiers
    } else {
        if from.fg != to.fg {
            stdout.queue(SetForegroundColor(Color::from(to.fg))).unwrap();
        }
        if from.bg != to.bg {
            stdout.queue(SetBackgroundColor(Color::from(to.bg))).unwrap();
        }

        to.modifiers - from.modifiers
    };

    if added.contains(Modifiers::BOLD) {
        stdout.queue(SetAttribute(Attribute::Bold)).unwrap();
    }
    if added.contains(Modifiers::UNDERLINE) {
        stdout.queue(SetAttribute(Attribute::Underlined)).unwrap();
    }
    if added.contains(Modifiers::BLINK) {
        stdout.queue(SetAttribute(Attribute::SlowBlink)).unwrap();
    }
    if added.contains(Modifiers::REVERSE) {
        stdout.queue(SetAttribute(Attribute::Reverse)).unwrap();
    }
}

#[derive(Copy, Clone)]
struct Glyph {
    ch: char,
    fg: Rgb,
    bg: Rgb,
    modifiers: Modifiers,
}

impl Default for Glyph {
    fn default() -> Self {
        Glyph { ch: ' ', fg: Rgb::new(0), bg: Rgb::new(0), modifiers: Modifiers::NONE }
    }
}

#[derive(Copy, Clone, PartialEq)]
pub struct Rgb {
    r: u8,
    g: u8,
    b: u8,
}

impl Rgb {
    pub fn new(hex: u32) -> Self {
        Self { r: (hex >> 16 & 0xff) as u8, g: (hex >> 8 & 0xff) as u8, b: (hex & 0xff) as u8 }
    }

    pub fn black() -> Self {
        Self::new(0)
    }
}

impl From<Rgb> for Color {
    fn from(rgb: Rgb) -> Self {
        Color::Rgb { r: rgb.r, g: rgb.g, b: rgb.b }
    }
}

bitflags::bitflags! {
    pub struct Modifiers: u8 {
        const NONE = 0;
        const BOLD = 1 << 0;
        const UNDERLINE = 1 << 1;
        const BLINK = 1 << 2;
        const REVERSE = 1 << 3;
    }
}
