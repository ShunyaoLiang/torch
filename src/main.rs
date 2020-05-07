mod ui;

fn main() {
    let mut ui = ui::Ui::new(ui::Rgb::from(0x0b302c));

    let test_windows = [ui::Window {
        lines: 18,
        cols: 32,
        line: 0,
        col: 0,
        draw: Box::new(|ui, window| {
            ui.draw(window, 0, 0, "Tes\nt", ui::Rgb::from(0xff5000), None);
        }),
    }];
    ui.render(&test_windows);

    std::io::stdin().read_line(&mut String::new());
}
