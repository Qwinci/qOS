#![allow(dead_code)]

use core::panic::PanicInfo;
use crate::allocator::init_memory;
use crate::limine::{FramebufferRequest, HHDMRequest, KernelAddressRequest, MemMapRequest, ModuleRequest, RSDPRequest};
use crate::{exit_qemu, main, println, QemuExitCode};

#[cfg(not(test))]
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
	println!("{}", info);
	loop {}
}

#[cfg(test)]
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
	serial_println!("[failed]\n");
	serial_println!("Error: {}", info);
	exit_qemu(QemuExitCode::Failed);
	loop {}
}

pub static MEM_MAP_REQUEST: MemMapRequest = MemMapRequest::new();
pub static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();
static RSDP_REQUEST: RSDPRequest = RSDPRequest::new();
static KERNEL_ADDRESS_REQUEST: KernelAddressRequest = KernelAddressRequest::new();
pub static MODULE_REQUEST: ModuleRequest = ModuleRequest::new();
pub static HHDM_REQUEST: HHDMRequest = HHDMRequest::new();
pub const FONT: &str = "Tamsyn8x16r.psf";

#[no_mangle]
extern "C" fn _start() {
	println!("hello {}", 20);
	init_memory();
	println!("hello {}", 20);

	main();
}
