pub static mut HIGH_HALF_OFFSET: usize = 0;
pub const SIZE_2MB: usize = 0x200000;

#[inline(always)]
pub fn set_high_half_offset(offset: usize) {
	unsafe {
		HIGH_HALF_OFFSET = offset;
	}
}

#[inline(always)]
pub fn get_high_half_offset() -> usize {
	unsafe {
		HIGH_HALF_OFFSET
	}
}