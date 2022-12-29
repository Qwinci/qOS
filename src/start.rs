use core::panic::PanicInfo;
use crate::allocator::init_memory;
use crate::limine::{FramebufferRequest, KernelAddressRequest, MemMapRequest, ModuleRequest, RSDPRequest};
use crate::{main, println};

#[cfg(not(test))]
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
	println!("{}", info);
	loop {}
}

pub static MEM_MAP_REQUEST: MemMapRequest = MemMapRequest::new();
pub static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();
static RSDP_REQUEST: RSDPRequest = RSDPRequest::new();
static KERNEL_ADDRESS_REQUEST: KernelAddressRequest = KernelAddressRequest::new();
pub static MODULE_REQUEST: ModuleRequest = ModuleRequest::new();
pub const FONT: &str = "Tamsyn8x16r.psf";

#[cfg(not(test))]
#[no_mangle]
extern "C" fn _start() {
	println!("hello {}", 20);
	init_memory();

	main();

	loop {}
}
