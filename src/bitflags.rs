#[macro_export]
macro_rules! bitflags {
	(field $vis:vis, $field_name:ident, bool, $range:expr, $t:ty, $Self:ident) => {
		qos_macros::replace! {
			$vis const $field_name: Self = Self::from(Self::start_end_to_mask(($range).start, ($range).end));
			$vis fn [<get_ $field_name>](&self) -> bool {
				const MASK: $t = ($Self::$field_name).into_inner();

				((self.value & MASK) >> ($range).start) != 0
			} }

		qos_macros::replace! { $vis fn [<set_ $field_name>](&mut self, value: bool) {
			const MASK: $t = ($Self::$field_name).into_inner();

			let value = ((value as $t) & (MASK >> ($range).start)) << ($range).start;

			self.value &= !MASK;
			self.value |= value;
		} }
	};
	(field $vis:vis, $field_name:ident, $field_type:ty, $range:expr, $t:ty, $Self:ident) => {
		qos_macros::replace! {
			$vis const $field_name: Self = Self::from($Self::start_end_to_mask(($range).start, ($range).end));
			$vis fn [<get_ $field_name>](&self) -> $field_type {
				const MASK: $t = ($Self::$field_name).into_inner();

				((self.value & MASK) >> ($range).start) as $field_type
			}
		}

		qos_macros::replace! { $vis fn [<set_ $field_name>](&mut self, value: $field_type) {
			const MASK: $t = ($Self::$field_name).into_inner();

			let value = ((value as $t) & (MASK >> ($range).start)) << ($range).start;

			self.value &= !MASK;
			self.value |= value;
		} }
	};
	(mask $range:expr, $t:ty) => {{
		Self::start_end_to_mask(($range).start, ($range).end)
	}};
    (
	    $(#[$attrib:meta])*
	    $vis:vis struct $struct_name:ident: $t:ty {
		    $($field_name:ident = $field_type:tt, $range:expr;)*
	    }
    ) => {
		$(#[$attrib])*
		#[repr(transparent)]
	    $vis struct $struct_name {
			value: $t
		}

	    impl $struct_name {
			$vis const fn new() -> Self {
				Self { value: 0 }
			}

			$vis const fn from(value: $t) -> Self {
				Self { value }
			}

			$vis const fn from_bits_truncated(value: $t) -> Self {
				Self { value: value & Self::MASK }
			}

			$vis const fn into_inner(self) -> $t {
				self.value
			}

			const fn start_end_to_mask(start: $t, end: $t) -> $t {
				#[cfg(not(any(windows, unix)))]
				use core::mem;
				#[cfg(any(windows, unix))]
				use std::mem;
				if end as usize - 1 >= mem::size_of::<$t>() * 8 {
					panic!("too large shift");
				}

				let mut mask = 0;
				let mut i = start;
				while i < end {
					mask |= 1 << i;
					i += 1;
				}
				mask
			}

			const fn generate_mask() -> $t {
				0 $(| bitflags!(mask $range, $t))*
			}

			const MASK: $t = Self::generate_mask();

			$(
			bitflags!(field $vis, $field_name, $field_type, $range, $t, $struct_name);
			)*
		}

	    impl core::ops::BitOr for $struct_name {
			type Output = Self;

			fn bitor(self, rhs: Self) -> Self::Output {
				Self::from(self.value | rhs.value)
			}
		}

		impl core::ops::BitAnd for Flags {
			type Output = Self;

			fn bitand(self, rhs: Self) -> Self::Output {
				Self::from(self.value | rhs.value)
			}
		}
    };
}