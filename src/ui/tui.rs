#[path = "../grid.rs"]
mod grid;

use grid::Grid;
use super::{
    Ui,
    Buffer,
    Component,
    Rgb,
    Attr,
    Event,
};

use term::terminfo::TermInfo;
use termios::{
    Termios,
    tcsetattr,
    TCSANOW,
};

use std::io::{
    stdout,
    stdin,
    Read,
};

pub struct Tui {
    buffer: TuiBuffer,
    terminfo: TermInfo,
    old_termios: Termios,
}

impl Tui {
    fn flush(&self) {
        use std::io::Write;

        let _ = self.terminfo.apply_cap("home", &[], &mut stdout());

        set_fg_rgb(self.buffer.first().fg);
        set_bg_rgb(self.buffer.first().bg);
        print!("{}", &self.buffer.first().ch);

        for pair in self.buffer.windows(2) {
            update_term_attrs(&pair[0], &pair[1], &self.terminfo);
            print!("{}", &pair[1].ch);
        }

        let _ = stdout().flush();
    }
}

impl Ui for Tui {
    fn new() -> Self {
        let terminfo = get_terminfo();
        let _ = terminfo.apply_cap("smcup", &[], &mut stdout());
        let _ = terminfo.apply_cap("civis", &[], &mut stdout());
        let _ = terminfo.apply_cap("sgr0", &[], &mut stdout());

        let old_termios = Termios::from_fd(0).unwrap();
        let mut new_termios = old_termios;
        termios::cfmakeraw(&mut new_termios);
        let _ = tcsetattr(0, TCSANOW, &new_termios);

        Self { buffer: TuiBuffer::new(terminal_size()), terminfo, old_termios, }
    }

    fn render(&mut self, components: &[Component]) {
        self.buffer.resize(terminal_size());
        self.buffer.clear();

        for component in components {
            (component.view)(&mut self.buffer, component);
            component.draw_border(&mut self.buffer);
        }

        self.flush();
    }

    fn size(&self) -> (usize, usize) {
        self.buffer.size()
    }

    fn poll_event(&mut self) -> Event {
        let mut buffer = [0u8; 1];
        let _ = stdin().read_exact(&mut buffer);
        Event::Key(buffer[0])
    }
}

impl Drop for Tui {
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

pub struct TuiBuffer(Grid<Glyph>);

impl TuiBuffer {
    fn new(size: (usize, usize)) -> Self {
        Self(Grid::new(size, Glyph::new()))
    }

    fn resize(&mut self, size: (usize, usize)) {
        self.0.resize(size, Glyph::new());
    }

    fn clear(&mut self) {
        self.0.fill(Glyph::new());
    }

    fn size(&self) -> (usize, usize) {
        self.0.size()
    }

    fn first(&self) -> &Glyph {
        &self.0[(0, 0)]
    }

    fn windows(&self, size: usize) -> std::slice::Windows<Glyph> {
        self.0.windows(size)
    }
}

impl Buffer for TuiBuffer {
    fn draw_ch(&mut self, ch: char, (line, col): (u16, u16), fg: Rgb, bg: Option<Rgb>, attr: Attr) {
        if !self.0.in_bounds((line as usize, col as usize)) {
            return;
        }

        let glyph = &mut self.0[(line as usize, col as usize)];
        glyph.ch = ch;
        glyph.fg = fg;
        glyph.bg = bg.unwrap_or(glyph.bg);
        glyph.attr = attr;
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
