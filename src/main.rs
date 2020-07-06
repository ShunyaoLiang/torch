mod ui;

use rand::random;

use std::time::Duration;

use ui::{Canvas, Color, Element, Glimmer, Modifiers, Ui};

fn main() {
	let ui = Ui::new();
	ignite(ui);
}

fn ignite(mut ui: Ui) {
	ui.render(|mut canvas| {
		let text = "This was drawn directly";
		canvas.draw(text).at((1, 1)).fg(Color::new(0xa3278f)).modifiers(Modifiers::BOLD).ink();
		canvas.draw('!').at((1, 24)).fg(Color::new(0xa3278f)).modifiers(Modifiers::BOLD).ink();
		canvas.draw_element(TestElement);
	});
	ui.read_event();

	let test_glimmer = Glimmer::new(
		Duration::from_millis(100),
		Box::new(|mut canvas| {
			if random::<u8>() % 2 == 0 {
				canvas.lighten(0.2, (1, 1));
			} else {
				canvas.darken(0.2, (1, 1));
			}
		}),
	);
	ui.glimmers.push(test_glimmer);
	ui.read_event();

	let text = "Typewriter effect!";
	ui.typewrite(text, (10, 1), Color::new(0xffffff), Color::default(), Modifiers::NONE);
	ui.read_event();
}

struct TestElement;

impl Element for TestElement {
	fn view(&mut self, canvas: &mut Canvas) {
		let text = "This was drawn by an element!";
		canvas.draw(text).at((2, 1)).fg(Color::new(0xff5000)).modifiers(Modifiers::BLINK).ink();

		let text = "Line wrapping!";
		canvas.draw(text).at((4, 2)).max_width(3).fg(Color::new(0x392dd1)).ink();
	}
}
