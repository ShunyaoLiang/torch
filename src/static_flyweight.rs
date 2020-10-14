#[macro_export]
macro_rules! static_flyweight {
	(
		$(#[$attr:meta])*
		$v:vis enum $id_name:ident -> $flyweight_type:ident {
			$($variant:ident = $val:tt),*$(,)?
		}
	) => {
		$(#[$attr])*
		#[derive(Clone, Copy, Eq, PartialEq)]
		$v enum $id_name {
			$($variant),*
		}

		use counted_array::counted_array;
		impl $id_name {
			counted_array!(const FLYWEIGHT_TABLE: [$flyweight_type; _] = [
				$($flyweight_type $val),*
			]);

			fn flyweight(&self) -> &$flyweight_type {
				&Self::FLYWEIGHT_TABLE[*self as usize]
			}
		}
	}
}
