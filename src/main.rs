#[macro_use]
extern crate bitflags;

mod ui;

fn main() {
    let mut ui = ui::Ui::new();

    let test_windows = [ui::Window {
        lines: 18,
        cols: 32,
        line: 0,
        col: 0,
        update: |window, ui| {
            window.draw_ch(ui, 1, 1, '@', ui::Rgb::new(0xff5000), None, ui::Attr::NONE);
        },
    }];

    ui.render(&test_windows);

    let event = ui.poll_event();
}
