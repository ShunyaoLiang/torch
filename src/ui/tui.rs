use term::terminfo::TermInfo;

use termios::{
    Termios,
    tcsetattr,
    TCSANOW,
};

use std::collections::HashMap;
use std::io::{
    stdout,
    stdin,
    Read,
};
use std::ops;

pub struct Ui {
    buffer: Buffer,
    terminfo: TermInfo,
    old_termios: Termios,
}

impl Ui {
    pub fn new() -> Self {
        let terminfo = get_terminfo();
        let _ = terminfo.apply_cap("smcup", &[], &mut stdout());
        let _ = terminfo.apply_cap("civis", &[], &mut stdout());
        let _ = terminfo.apply_cap("sgr0", &[], &mut stdout());

        let old_termios = Termios::from_fd(0).unwrap();
        let mut new_termios = old_termios;
        termios::cfmakeraw(&mut new_termios);
        let _ = tcsetattr(0, TCSANOW, &new_termios);

        Self { buffer: Buffer::new(terminal_size()), terminfo, old_termios, }
    }

    pub fn render(&mut self, windows: &[Window]) {
        self.buffer.resize(terminal_size());
        self.buffer.clear();

        for window in windows {
            (window.update)(window, self);
            window.draw_border(self);
        }

        self.buffer.flush(&self.terminfo);
    }

    pub fn poll_event(self) -> InputEvent {
        let mut buffer = [0u8; 1];
        let _ = stdin().read_exact(&mut buffer);
        InputEvent::Key(buffer[0])
    }
}

impl Drop for Ui {
    fn drop(&mut self) {
        let _ = self.terminfo.apply_cap("cvvis", &[], &mut stdout());
        let _ = self.terminfo.apply_cap("rmcup", &[], &mut stdout());
        let _ = self.terminfo.apply_cap("sgr0", &[], &mut stdout());

        let _ = tcsetattr(0, TCSANOW, &self.old_termios);
    }
}

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

fn get_terminfo() -> TermInfo {
    use std::env::var;

    let term = var("TERM").expect("Failed to detect your terminal. Is TERM set?");
    TermInfo::from_name(&term).unwrap_or_else(|_| -> TermInfo {
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
    })
}

pub struct Window {
    pub line: usize,
    pub col: usize,
    pub lines: usize,
    pub cols: usize,
    pub update: fn(&Window, &mut Ui),
}

impl Window {
    pub fn draw_str(
        &self,
        ui: &mut Ui,
        line: usize,
        col: usize,
        str: &str,
        fg: Rgb,
        bg: Option<Rgb>,
        attr: Attr,
    ) {
        ui.buffer.draw_str(line + self.line, col + self.col, str, fg, bg, attr);
    }

    pub fn draw_ch(
        &self,
        ui: &mut Ui,
        line: usize,
        col: usize,
        ch: char,
        fg: Rgb,
        bg: Option<Rgb>,
        attr: Attr,
    ) {
        ui.buffer.draw_ch(line + self.line, col + self.col, ch, fg, bg, attr);
    }

    fn draw_border(&self, ui: &mut Ui) {
        let white: Rgb = Rgb::new(0xcccccc);

        for line in 0..self.lines {
            let (left_ch, right_ch) = match line {
                0 => ('┌', '┐'),
                l if l == self.lines - 1 => ('└', '┘'),
                _ => ('│', '│'),
            };
            self.draw_ch(ui, line, 0, left_ch, white, None, Attr::NONE);
            self.draw_ch(ui, line, self.cols - 1, right_ch, white, None, Attr::NONE);
        }
        for col in 1..self.cols - 1 {
            self.draw_ch(ui, 0, col, '─', white, None, Attr::NONE);
            self.draw_ch(ui, self.lines - 1, col, '─', white, None, Attr::NONE);
        }
    }
}

struct Buffer {
    contents: Vec<Glyph>,
    lines: usize,
    cols: usize,
}

impl Buffer {
    fn new((lines, cols): (usize, usize)) -> Self {
        Self { contents: vec![Glyph::new(); lines * cols], lines, cols }
    }

    fn resize(&mut self, (new_lines, new_cols): (usize, usize)) {
        use std::ptr::copy;

        unsafe {
            let vec = self.contents.as_mut_ptr();
            let cols = self.cols;
            let move_line = |line: usize| {
                let src = vec.offset((line * cols) as isize);
                let dest = src.offset(new_cols as isize - cols as isize);
                copy(src, dest, cols);
            };

            if new_cols > self.cols {
                self.contents.resize(new_lines * new_cols, Glyph::new());
                (1..self.lines).rev().for_each(move_line);
            } else if new_cols < self.cols {
                (1..self.lines).for_each(move_line);
                self.contents.resize(new_lines * new_cols, Glyph::new());
            }
        }
    }

    fn clear(&mut self) {
        for glyph in &mut self.contents {
            *glyph = Glyph::new();
        }
    }

    fn in_bounds(&self, line: usize, col: usize) -> bool {
        line < self.lines && col < self.cols
    }

    fn draw_str(
        &mut self,
        line: usize,
        col: usize,
        str: &str,
        fg: Rgb,
        bg: Option<Rgb>,
        attr: Attr,
    ) {
        for (pos, ch) in str.chars().enumerate() {
            if ch == '\n' {
                self.draw_str(line + 1, col, &str[pos + 1..], fg, bg, attr);
                break;
            }

            self.draw_ch(line, col + pos, ch, fg, bg, attr);
        }
    }

    fn draw_ch(&mut self, line: usize, col: usize, ch: char, fg: Rgb, bg: Option<Rgb>, attr: Attr) {
        if !self.in_bounds(line, col) {
            return;
        }

        let glyph = &mut self[(line, col)];
        glyph.ch = ch;
        glyph.fg = fg;
        glyph.bg = bg.unwrap_or(glyph.bg);
        glyph.attr = attr;
    }

    fn flush(&mut self, terminfo: &TermInfo) {
        use std::io::Write;

        let _ = terminfo.apply_cap("home", &[], &mut stdout());

        set_fg_rgb(self.contents[0].fg);
        set_bg_rgb(self.contents[0].bg);
        print!("{}", &self.contents[0].ch);

        for pair in self.contents.windows(2) {
            update_term_attrs(&pair[0], &pair[1], terminfo);
            print!("{}", &pair[1].ch);
        }

        let _ = stdout().flush();
    }
}

fn update_term_attrs(last: &Glyph, curr: &Glyph, terminfo: &TermInfo) {
    if curr.attr.contains(last.attr) {
        set_term_attrs(curr.attr - last.attr, terminfo);
        if curr.fg != last.fg {
            set_fg_rgb(curr.fg);
        }
        if curr.bg != last.bg {
            set_bg_rgb(curr.bg);
        }
    } else {
        let _ = terminfo.apply_cap("sgr0", &[], &mut stdout());
        set_fg_rgb(curr.fg);
        set_bg_rgb(curr.bg);
        set_term_attrs(curr.attr, terminfo);
    }
}

fn set_term_attrs(attr: Attr, terminfo: &TermInfo) {
    if attr.contains(Attr::BOLD) {
        let _ = terminfo.apply_cap("bold", &[], &mut stdout());
    }
    if attr.contains(Attr::UNDERLINE) {
        let _ = terminfo.apply_cap("smul", &[], &mut stdout());
    }
    if attr.contains(Attr::BLINK) {
        let _ = terminfo.apply_cap("blink", &[], &mut stdout());
    }
    if attr.contains(Attr::REVERSE) {
        let _ = terminfo.apply_cap("rev", &[], &mut stdout());
    }
}

fn set_fg_rgb(rgb: Rgb) {
    print!("\x1b[38;2;{};{};{}m", rgb.r, rgb.g, rgb.b);
}

fn set_bg_rgb(rgb: Rgb) {
    print!("\x1b[48;2;{};{};{}m", rgb.r, rgb.g, rgb.b);
}

impl ops::Index<(usize, usize)> for Buffer {
    type Output = Glyph;

    fn index(&self, (line, col): (usize, usize)) -> &Self::Output {
        if !self.in_bounds(line, col) {
            panic!(
                "pos out of bounds: the pos is {:?} but the size is {:?}",
                (line, col),
                (self.lines, self.cols)
            )
        }

        &self.contents[line * self.cols + col]
    }
}

impl ops::IndexMut<(usize, usize)> for Buffer {
    fn index_mut(&mut self, (line, col): (usize, usize)) -> &mut Self::Output {
        if !self.in_bounds(line, col) {
            panic!(
                "pos out of bounds: the pos is {:?} but the size is {:?}",
                (line, col),
                (self.lines, self.cols)
            )
        }

        &mut self.contents[line * self.cols + col]
    }
}

#[derive(Copy, Clone)]
struct Glyph {
    ch: char,
    fg: Rgb,
    bg: Rgb,
    attr: Attr,
}

impl Glyph {
    fn new() -> Self {
        Glyph { ch: ' ', fg: Rgb::new(0), bg: Rgb::new(0), attr: Attr::NONE }
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

pub enum InputEvent {
    Key(u8),
}
