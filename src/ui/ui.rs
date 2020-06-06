use crate::ui::buffer::{Buffer, Modifiers, Rgb};

use crossterm::{
    cursor, event, execute, queue,
    style::{
        Attribute::{Bold, Reset, Reverse, SlowBlink, Underlined},
        Color, Print, SetAttribute, SetBackgroundColor, SetForegroundColor,
    },
    terminal::{self, Clear, ClearType, EnterAlternateScreen, LeaveAlternateScreen},
    QueueableCommand,
};

use std::fmt::Display;
use std::io::{self, Write};

pub struct Ui<'a> {
    components: &'a [Component<'a>],
    buffer: Buffer,
}

impl<'a> Ui<'a> {
    pub fn new() -> Self {
        execute!(io::stdout(), SetAttribute(Reset), cursor::Hide, EnterAlternateScreen,).unwrap();

        Self { components: &[], buffer: Buffer::new(terminal_size()) }
    }

    pub fn set_components(&mut self, components: &'a [Component<'a>]) {
        self.components = components;
    }

    pub fn render_components(&mut self) {
        let mut stdout = io::stdout();

        self.buffer.resize(terminal_size());

        stdout.queue(Clear(ClearType::All)).unwrap();
        for component in self.components {
            (component.view)(self.pen_for(component));
        }
        self.buffer.flush();
    }

    pub fn poll_event(&self) -> event::Event {
        event::read().unwrap()
    }

    fn pen_for(&mut self, component: &Component) -> Pen {
        Pen {
            buffer: &mut self.buffer,
            line: component.line,
            col: component.col,
            lines: component.lines,
            cols: component.cols,
        }
    }
}

impl Drop for Ui<'_> {
    fn drop(&mut self) {
        execute!(io::stdout(), SetAttribute(Reset), cursor::Show, LeaveAlternateScreen,).unwrap();
    }
}

pub struct Component<'ui> {
    line: u16,
    col: u16,
    lines: u16,
    cols: u16,
    view: Box<dyn Fn(Pen) + 'ui>,
}

impl<'ui> Component<'ui> {
    pub fn new<F>((line, col): (u16, u16), (lines, cols): (u16, u16), view: F) -> Self
    where
        F: Fn(Pen) + 'ui,
    {
        Component { lines, cols, line, col, view: Box::new(view) }
    }
}

pub struct Pen<'buf> {
    buffer: &'buf mut Buffer,
    line: u16,
    col: u16,
    lines: u16,
    cols: u16,
}

impl<'buf> Pen<'buf> {
    /// Not the curtain kind.
    pub fn draw_str(
        &mut self, text: &str, (line, col): (u16, u16), fg: Rgb, bg: Rgb, modifiers: Modifiers,
    ) {
        for (pos, ch) in text.chars().enumerate() {
            if ch == '\n' {
                self.draw_str(&text[pos + 1..], (line + 1, col), fg, bg, modifiers);
                break;
            } else if col + pos as u16 > self.cols {
                // Wrap text if we have the lines to do so
                if line < self.lines - 1 {
                    self.draw_str(&text[pos..], (line + 1, col), fg, bg, modifiers);
                }
                break;
            }

            self.draw(ch, (line, col + pos as u16), fg, bg, modifiers);
        }
    }

    pub fn draw(
        &mut self, ch: char, (line, col): (u16, u16), fg: Rgb, bg: Rgb, modifiers: Modifiers,
    ) {
        self.buffer.draw(ch, (line + self.line, col + self.col), fg, bg, modifiers);
    }
}

fn terminal_size() -> (usize, usize) {
    let (cols, lines) = terminal::size().expect("Failed to get terminal size");
    (lines as usize, cols as usize)
}
