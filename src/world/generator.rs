use super::RegionKey;
use super::World;
use super::region::Region;

use rand::Rng;

pub trait Generator {
	fn generate_terrain(&mut self, rng: &mut impl Rng) -> Region;
	fn generate_entities(&mut self, world: &mut World, key: RegionKey);
}
