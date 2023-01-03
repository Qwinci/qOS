use core::arch::asm;
use crate::println;

pub static mut HIGH_HALF_OFFSET: usize = 0;
pub const KERNEL_BASE: usize = 0xFFFFFFFF80000000;
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

#[repr(C)]
struct StackFrame {
	rbp: *const StackFrame,
	rip: u64
}

pub fn unwind() {
	let mut frame: *const StackFrame;
	unsafe {
		asm!("mov {}, rbp", out(reg) frame);
	}
	println!("stack trace:\n");
	while !frame.is_null() {
		unsafe {
			println!("{:#X}", (*frame).rip);
			frame = (*frame).rbp;
		}
	}
}