#![feature(pointer_byte_offsets)]
#![feature(once_cell)]
#![feature(abi_x86_interrupt)]
#![feature(allocator_api)]
#![no_std]
#![no_main]

extern crate alloc;

use core::arch::asm;
use core::marker::PhantomData;

mod limine;
mod sync;
mod fb;
mod start;
mod boot;
mod allocator;
mod utils;
mod paging;
mod bitflags;
mod x86;
mod interrupts;

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u32)]
pub enum QemuExitCode {
	Success = 0x10,
	Failed = 0x11
}

trait PortReadWrite {
	unsafe fn read_from_port(port: u16) -> Self;
	unsafe fn write_to_port(port: u16, value: Self);
}

impl PortReadWrite for u8 {
	#[inline]
	unsafe fn read_from_port(port: u16) -> Self {
		let value: u8;
		asm!("in al, dx", in("dx") port, out("al") value);
		value
	}

	unsafe fn write_to_port(port: u16, value: Self) {
		asm!("out dx, al", in("dx") port, in("al") value)
	}
}

impl PortReadWrite for u16 {
	#[inline]
	unsafe fn read_from_port(port: u16) -> Self {
		let value: u16;
		asm!("in ax, dx", in("dx") port, out("ax") value);
		value
	}

	unsafe fn write_to_port(port: u16, value: Self) {
		asm!("out dx, ax", in("dx") port, in("ax") value)
	}
}

impl PortReadWrite for u32 {
	#[inline]
	unsafe fn read_from_port(port: u16) -> Self {
		let value: u32;
		asm!("in eax, dx", in("dx") port, out("eax") value);
		value
	}

	unsafe fn write_to_port(port: u16, value: Self) {
		asm!("out dx, eax", in("dx") port, in("eax") value)
	}
}

struct Port<T: PortReadWrite> {
	port: u16,
	_phantom: PhantomData<T>
}

impl<T: PortReadWrite> Port<T> {
	pub const unsafe fn new(port: u16) -> Self {
		Self { port, _phantom: PhantomData }
	}

	#[inline]
	pub unsafe fn read(&self) -> T {
		T::read_from_port(self.port)
	}

	#[inline]
	pub unsafe fn write(&mut self, value: T) {
		T::write_to_port(self.port, value)
	}
}

pub fn exit_qemu(exit_code: QemuExitCode) {
	unsafe {
		let mut port = Port::new(0xf4);
		port.write(exit_code as u32);
	}
}

pub fn main() -> ! {
	loop {}
}