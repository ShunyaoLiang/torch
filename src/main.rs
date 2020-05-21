#[macro_use]
extern crate bitflags;

mod ui;

use ui::Ui;

use std::error::Error;

fn f(buffer: &mut dyn ui::Buffer, component: &ui::Component) {
    unsafe {
        static mut g: i64 = 0;
        if g % 2 == 0 {
            component.draw_str("Multi\nple\nLines", buffer, (2, 3), ui::Rgb::new(0xff5000), None, ui::Attr::NONE);
        } else {
            component.draw_str("cock", buffer, (2, 3), ui::Rgb::new(0xff5000), None, ui::Attr::NONE);
        }
        g += 1;
    }
}

fn main() {
    let ui;
    cfg_if::cfg_if! {
        if #[cfg(feature = "default")] {
            ui = ui::Tui::new();
        }
    }
    cfg_if::cfg_if! {
        if #[cfg(feature = "gui")] {
            ui = ui::Gui::new();
        }
    }
    let g = 0;
    let viewport_component = ui::Component::new((0, 0), (24, 80), f);
    let main_components = [viewport_component];

    if let Err(e) = run(ui, &main_components) {
        eprintln!("Runtime Error: {}", e);
    };
}

fn run<Ui>(mut ui: Ui, main_components: &[ui::Component]) -> Result<(), Box<dyn Error>>
    where Ui: ui::Ui {
    loop {
        ui.render(main_components);
//        let _ = ui.poll_event();
        let _ = std::io::stdin().read_line(&mut String::new());
    }
    
    Ok(())
}
