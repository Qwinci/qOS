#![allow(dead_code)]

use core::arch::{asm, global_asm};
use core::panic::PanicInfo;
use crate::allocator::init_memory;
use crate::limine::{FramebufferRequest, HHDMRequest, KernelAddressRequest, MemMapRequest, ModuleRequest, RSDPRequest};
use crate::{exit_qemu, main, println, QemuExitCode, x86};
use crate::fb::da;
use crate::interrupts::{Idt, IdtEntry, IdtFlags, InterruptFrame, PageFaultFlags, PageFaultHandler};
use crate::paging::{Flags, PhysAddr, VirtAddr};

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
pub static KERNEL_ADDRESS_REQUEST: KernelAddressRequest = KernelAddressRequest::new();
pub static MODULE_REQUEST: ModuleRequest = ModuleRequest::new();
pub static HHDM_REQUEST: HHDMRequest = HHDMRequest::new();
pub const FONT: &str = "Tamsyn8x16r.psf";

//static mut IDT: Idt = Idt::new();
static mut IDT: x86_64::structures::idt::InterruptDescriptorTable = x86_64::structures::idt::InterruptDescriptorTable::new();

extern "x86-interrupt" fn page_fault_handler(frame: x86_64::structures::idt::InterruptStackFrame, flags: x86_64::structures::idt::PageFaultErrorCode) {
	println!("page fault");
	loop {
		unsafe {
			asm!("hlt");
		}
	}
}

extern "x86-interrupt" fn double_fault_handler(frame: x86_64::structures::idt::InterruptStackFrame, code: u64) -> ! {
	println!("double fault");
	loop {
		unsafe {
			asm!("hlt");
		}
	}
}

const INT_GATE: u8 = 0b1110;
const TRAP_GATE: u8 = 0b1111;

struct GdtEntry {
	limit0: u16,
	base0: u16,
	base1: u8,
	access: u8,
	limit1_flags: u8,
	base2: u8
}

impl GdtEntry {
	const fn new(access: u8, flags: u8) -> Self {
		Self {
			limit0: 0,
			base0: 0,
			base1: 0,
			access,
			limit1_flags: flags << 4,
			base2: 0
		}
	}
}

static mut GDT: [GdtEntry; 5] = [
	// null
	GdtEntry::new(0, 0),
	// kernel code
	GdtEntry::new(0x9A, 0xA),
	// kernel data
	GdtEntry::new(0x92, 0xC),
	// user code
	GdtEntry::new(0xFA, 0xA),
	// user data
	GdtEntry::new(0xF2, 0xC)
];

#[repr(packed)]
struct Gdtr {
	size: u16,
	offset: *mut GdtEntry
}

global_asm!(
	r"
	.global load_gdt_asm
	load_gdt_asm:
		lgdt [rdi]
		mov ax, 0x10
		mov ds, ax
		mov es, ax
		mov fs, ax
		mov gs, ax
		mov ss, ax
		pop rdi
		mov rax, 0x8
		push rax
		push rdi
		retfq
	"
);

extern "sysv64" {
	fn load_gdt_asm(gdtr: &Gdtr);
}

fn load_gdt() {
	let gdtr = Gdtr {
		size: core::mem::size_of_val(unsafe { &GDT }) as u16 - 1,
		offset: unsafe { GDT.as_mut_ptr() }
	};
	unsafe {
		load_gdt_asm(&gdtr);
	}
}

#[no_mangle]
extern "C" fn _start() {
	println!("hello {}", 20);
	da();

	unsafe {
		asm!("cli");
	}
	load_gdt();

	unsafe {
		//let mut flags = IdtFlags::new();
		//flags.set_present(true);
		//flags.set_gate_type(TRAP_GATE);
		/*IDT.page_fault = IdtEntry::new(page_fault_handler, 0x8, 0, flags);
		IDT.double_fault = IdtEntry::new(double_fault_handler, 0x8, 0, flags);
		IDT.load();*/
		IDT.page_fault.set_handler_fn(page_fault_handler);
		IDT.double_fault.set_handler_fn(double_fault_handler);
		IDT.load();

		asm!("sti");
		println!("interrupts enabled");
	}

	init_memory();
	println!("hello {}", 20);

	main();
}
