use crate::world::Point;
use crate::world::Region;

use rand::Rng;

pub mod caves;

pub use caves::Caves;

pub fn random_empty_tile(region: &Region, rng: &mut impl Rng) -> Point {
	for _ in 0..200 {
		let pos = random_point(rng);
		if region[pos].held_entity.is_none() && !region[pos].blocks() {
			return pos;
		}
	}

	panic!("wow we couldn't find an empty tile after 200 attempts");
}

fn random_point(rng: &mut impl Rng) -> Point {
	(rng.gen_range(0, Region::WIDTH), rng.gen_range(0, Region::HEIGHT)).into()
}
