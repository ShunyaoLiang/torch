use super::Direction;
use super::RegionKey;
use super::World;
use super::region::Region;

use rand::Rng;

pub trait Generator: Clone {
	fn generate_terrain(&mut self, rng: &mut impl Rng) -> Region;
	fn generate_entities(&mut self, world: &mut World, key: RegionKey);
	fn generate_connection(&mut self, direction: Direction);
}
