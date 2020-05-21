cfg_if::cfg_if! {
    if #[cfg(feature = "default")] {
        mod tui;
        pub use tui::Tui;
    }
}
cfg_if::cfg_if! {
    if #[cfg(feature = "gui")] {
        mod gui;
        pub use gui::Gui;
    }
}

pub trait Ui {
    fn new() -> Self;
    fn render(&mut self, components: &[Component]);
    fn size(&self) -> (usize, usize);
    fn poll_event(&self) -> Event;
}

pub trait Buffer {
    fn draw_ch(&mut self, ch: char, pos: (u16, u16), fg: Rgb, bg: Option<Rgb>, attr: Attr);

    /// Not the curtain kind.
    fn draw_str(&mut self, str: &str, (line, col): (u16, u16), fg: Rgb, bg: Option<Rgb>, attr: Attr) {
        for (pos, ch) in str.chars().enumerate() {
            if ch == '\n' {
                self.draw_str(&str[pos + 1..], (line + 1, col), fg, bg, attr);
                break;
            }

            self.draw_ch(ch, (line, col + pos as u16), fg, bg, attr);
        }
    }
}

pub struct Component {
    line: u16, col: u16,
    lines: u16, cols: u16,
    view: fn(&mut dyn Buffer, &Component),
}

impl Component {
    pub fn new((line, col): (u16, u16), (lines, cols): (u16, u16), view: fn(&mut dyn Buffer, &Component)) -> Self {
        Component { line, col, lines, cols, view }
    }

    pub fn draw_ch(
        &self,
        ch: char,
        buffer: &mut dyn Buffer,
        (line, col): (u16, u16),
        fg: Rgb,
        bg: Option<Rgb>,
        attr: Attr
    ) {
        buffer.draw_ch(ch, (line + self.line, col + self.col), fg, bg, attr);
    }

    pub fn draw_str(
        &self,
        str: &str,
        buffer: &mut dyn Buffer,
        (line, col): (u16, u16),
        fg: Rgb,
        bg: Option<Rgb>,
        attr: Attr,
    ) {
        buffer.draw_str(str, (line + self.line, col + self.col), fg, bg, attr);
    }

    fn draw_border(&self, buffer: &mut dyn Buffer) {
        let white: Rgb = Rgb::new(0xcccccc);

        for line in 0..self.lines {
            let (left_ch, right_ch) = match line {
                0 => ('┌', '┐'),
                l if l == self.lines - 1 => ('└', '┘'),
                _ => ('│', '│'),
            };
            self.draw_ch(left_ch, buffer, (line, 0), white, None, Attr::NONE);
            self.draw_ch(right_ch, buffer, (line, self.cols - 1), white, None, Attr::NONE);
        }
        for col in 1..self.cols - 1 {
            self.draw_ch('─', buffer, (0, col), white, None, Attr::NONE);
            self.draw_ch('─', buffer, (self.lines - 1, col), white, None, Attr::NONE);
        }
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
}

bitflags! {
    pub struct Attr: u8 {
        const NONE = 0;
        const BOLD = 1 << 0;
        const UNDERLINE = 1 << 1;
        const BLINK = 1 << 2;
        const REVERSE = 1 << 3;
    }
}

pub enum Event {
    Key(u8),
}
