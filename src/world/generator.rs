use super::region::Region;

use rand::Rng;

pub trait Generator {
	fn generate(&mut self, rng: &mut impl Rng) -> Region;
}
