mod ui;

use ui::{Color, Modifiers, Pen, Ui};

fn main() {
    let mut ui = Ui::new();
    ui.render(|_, _| {
        let components: Vec<Box<dyn ui::Component>> = vec![Box::new(TestComponent)];
        components.into_boxed_slice()
    });
    ui.poll_event();
}

struct TestComponent;

impl ui::Component for TestComponent {
    fn bounds(&self) -> ui::Rectangle {
        ui::Rectangle::new((2, 0), (24, 80))
    }

    fn view(&mut self, mut pen: Pen) {
        pen.draw_text("Test", (0, 2), Color::new(0xff5000), Color::default(), Modifiers::BOLD);
    }
}
