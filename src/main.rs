mod grid;
mod ui;

use ui::Component;
use ui::Modifiers;
use ui::Rgb;
use ui::Ui;

use std::error::Error;

fn main() {
    let test_component = Component::new((3, 4), (24, 80), |mut pen| {
        pen.draw('@', (0, 0), Rgb::new(0xeedc67), Rgb::black(), Modifiers::UNDERLINE);
        pen.draw_str(
            "Very Long Test String",
            (1, 0),
            Rgb::new(0xeedc67),
            Rgb::black(),
            Modifiers::BLINK,
        );
    });
    let test_components = [test_component];
    let ui = Ui::with_components(&test_components);

    if let Err(e) = run(ui) {
        eprintln!("Runtime Error: {}", e);
    };
}

fn run(mut ui: ui::Ui) -> Result<(), Box<dyn Error>> {
    ui.render_components();
    ui.poll_event();

    Ok(())
}
