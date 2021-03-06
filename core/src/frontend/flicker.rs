use super::Event;
use super::Frontend;
use super::Result;
use super::Screen;

use crossterm::event::poll as poll_event;

use std::cmp::min;
use std::time::Duration;
use std::time::Instant;

macro_rules! unwrap_or_return {
	($option:expr,$default:expr) => {
		match $option {
			Some(value) => value,
			None => return $default,
		}
	};
}

impl Frontend<'_> {
	pub fn flicker(
		&mut self, mut script: impl FnMut(&mut Screen) -> Option<Duration>
	) -> Result<Option<Event>> {
		let mut render = |frontend: &mut Frontend| -> Result<Option<Duration>> {
			let mut buffer = frontend.buffer.clone();
			let timeout = script(&mut Screen::new(&mut buffer));
			buffer.flush(&mut frontend.stdout)?;
			Ok(timeout)
		};

		let mut timeout = unwrap_or_return!(render(self)?, Ok(None));

		loop {
			let now = Instant::now();
			match poll_event(timeout)? {
				true => {
					timeout -= min(now.elapsed(), timeout);
					return Ok(Some(self.read_event()?));
				}
				false => timeout = unwrap_or_return!(render(self)?, Ok(None)),
			}
		}
	}
}
