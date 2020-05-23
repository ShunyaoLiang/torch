use super::{
    Ui,
    Buffer,
    Component,
    Rgb,
    Attr,
    Event,
};

pub struct Gui {
    sdl: sdl2::Sdl,
    video: sdl2::VideoSubsystem,
    ttf: sdl2::ttf::Sdl2TtfContext,
    event_pump: sdl2::EventPump,
    canvas: sdl2::render::WindowCanvas,
}

impl Ui for Gui {
    fn new() -> Self {
        let sdl = sdl2::init().unwrap();
        let video = sdl.video().unwrap();
        let ttf = sdl2::ttf::init().unwrap();
        let event_pump = sdl.event_pump().unwrap();
        let window = video.window("Torch", 800, 600)
            .position_centered()
            .build()
            .unwrap();
        let mut canvas = window.into_canvas().build().unwrap();

        Self { sdl, video, ttf, event_pump, canvas, }
    }

    fn render(&mut self, components: &[Component]) {
        self.canvas.clear();
        // Thank Mayugoro, SDL_TTF caches fonts.
        let font = self.ttf.load_font("/usr/share/fonts/misc/cherry-13-r.otb", 13).unwrap();

        for component in components {
            let mut buffer = GuiBuffer { canvas: &mut self.canvas, font: &font };
            (component.view)(&mut buffer, component);
            component.draw_border(&mut buffer);
        }

        self.canvas.present();
    }

    fn size(&self) -> (usize, usize) {
        (24, 80)
    }

    fn poll_event(&mut self) -> Event {
        let event = self.event_pump.wait_event();
        if let sdl2::event::Event::KeyDown { keycode, .. } = event {
            match keycode.unwrap() {
                sdl2::keyboard::Keycode::H => Event::Key('h' as u8),
                sdl2::keyboard::Keycode::J => Event::Key('j' as u8),
                sdl2::keyboard::Keycode::K => Event::Key('k' as u8),
                sdl2::keyboard::Keycode::L => Event::Key('l' as u8),
                _ => Event::Key('_' as u8),
            }
        } else {
            self.poll_event()
        }
    }
}

struct GuiBuffer<'a> {
    canvas: &'a mut sdl2::render::WindowCanvas,
    font: &'a sdl2::ttf::Font<'a, 'a>,
}

impl Buffer for GuiBuffer<'_> {
    fn draw_ch(&mut self, ch: char, (line, col): (u16, u16), fg: Rgb, bg: Option<Rgb>, attr: Attr) {
        use sdl2::pixels::Color;

        if line >= 24 || col >= 80 {
            return;
        }

        // What am I drawing?
        let bg = bg.unwrap_or(Rgb::new(0));
        let surface = self.font.render_char(ch).shaded(Color::from(fg), Color::from(bg)).unwrap();
        let texture_creator = self.canvas.texture_creator();
        let texture = texture_creator.create_texture_from_surface(&surface).unwrap();
        // Where am I drawing it?
        let sdl2::render::TextureQuery { width, height, .. } = texture.query();
        let (mut font_width, mut font_height) = self.font.size_of_char('@').unwrap();
        let rect = sdl2::rect::Rect::new(col as i32 * font_width as i32, line as i32 * font_height as i32, width, height);
        // Draw!
        self.canvas.copy(&texture, None, rect).unwrap();
    }
}

impl From<Rgb> for sdl2::pixels::Color {
    fn from(rgb: Rgb) -> Self {
        Self::RGB(rgb.r, rgb.g, rgb.b)
    }
}
