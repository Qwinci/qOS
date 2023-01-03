use core::arch::asm;
use core::marker::PhantomData;
use crate::bitflags;

pub type Handler = extern "x86-interrupt" fn(frame: InterruptFrame);
pub type HandlerWithCode = extern "x86-interrupt" fn(frame: InterruptFrame, code: u64);

bitflags! {
	pub struct PageFaultFlags: u64 {
		present = bool, 0..1;
		write = bool, 1..2;
		user = bool, 2..3;
		reserved_write = bool, 3..4;
		instruction_fetch = bool, 4..5;
		protection_key = bool, 5..6;
		shadow_stack = bool, 6..7;
		software_guard_extensions = bool, 7..8;
	}
}

pub type PageFaultHandler = extern "x86-interrupt" fn(frame: InterruptFrame, flags: PageFaultFlags);

pub struct InterruptFrame {
	pub ip: u64,
	pub cs: u64,
	pub flags: u64,
	pub sp: u64,
	pub ss: u64
}

bitflags! {
	#[derive(Copy, Clone, Debug)]
	pub struct IdtFlags: u8 {
		gate_type = u8, 0..4;
		dpl = u8, 5..7;
		present = bool, 7..8;
	}
}

#[derive(Copy, Clone)]
pub struct IdtEntry<F> {
	offset0: u16,
	selector: u16,
	ist: u8,
	flags: IdtFlags,
	offset1: u16,
	offset2: u32,
	zero: u32,
	_phantom: PhantomData<F>
}

impl<F> IdtEntry<F> {
	pub fn new(handler: F, selector: u16, ist: u8, flags: IdtFlags) -> Self {
		let mut addr = &handler as *const _ as usize;

		let offset0 = (addr & u16::MAX as usize) as u16;
		addr >>= u16::BITS;
		let offset1 = (addr & u16::MAX as usize) as u16;
		addr >>= u16::BITS;
		let offset2 = (addr & u32::MAX as usize) as u32;

		Self {
			offset0,
			selector,
			ist,
			flags,
			offset1,
			offset2,
			zero: 0,
			_phantom: PhantomData
		}
	}

	pub const fn zero() -> Self {
		Self {
			offset0: 0,
			selector: 0,
			ist: 0,
			flags: IdtFlags::new(),
			offset1: 0,
			offset2: 0,
			zero: 0,
			_phantom: PhantomData
		}
	}
}

pub struct Idt {
	pub division_error: IdtEntry<Handler>,
	pub debug: IdtEntry<Handler>,
	pub nmi: IdtEntry<Handler>,
	pub breakpoint: IdtEntry<Handler>,
	pub overflow: IdtEntry<Handler>,
	pub bound_range_exceeded: IdtEntry<Handler>,
	pub invalid_opcode: IdtEntry<Handler>,
	pub device_not_available: IdtEntry<Handler>,
	pub double_fault: IdtEntry<HandlerWithCode>,
	_pad0: usize,
	pub invalid_tss: IdtEntry<HandlerWithCode>,
	pub seg_not_present: IdtEntry<HandlerWithCode>,
	pub stack_seg_fault: IdtEntry<HandlerWithCode>,
	pub gp_fault: IdtEntry<HandlerWithCode>,
	pub page_fault: IdtEntry<PageFaultHandler>,
	_pad1: usize,
	pub x86_float_exception: IdtEntry<Handler>,
	pub alignment_check: IdtEntry<HandlerWithCode>,
	pub machine_check: IdtEntry<Handler>,
	pub simd_float_exception: IdtEntry<Handler>,
	pub virtualization_exception: IdtEntry<Handler>,
	pub control_protection_exception: IdtEntry<HandlerWithCode>,
	_pad2: [usize; 6],
	pub hypervisor_injection_exception: IdtEntry<Handler>,
	pub vmm_communication_exception: IdtEntry<HandlerWithCode>,
	pub security_exception: IdtEntry<HandlerWithCode>,
	_pad3: usize,
	pub others: [IdtEntry<Handler>; 256 - 32]
}

struct Idtr {
	size: u16,
	offset: *const Idt
}

impl Idt {
	pub const fn new() -> Self {
		Self {
			division_error: IdtEntry::zero(),
			debug: IdtEntry::zero(),
			nmi: IdtEntry::zero(),
			breakpoint: IdtEntry::zero(),
			overflow: IdtEntry::zero(),
			bound_range_exceeded: IdtEntry::zero(),
			invalid_opcode: IdtEntry::zero(),
			device_not_available: IdtEntry::zero(),
			double_fault: IdtEntry::zero(),
			_pad0: 0,
			invalid_tss: IdtEntry::zero(),
			seg_not_present: IdtEntry::zero(),
			stack_seg_fault: IdtEntry::zero(),
			gp_fault: IdtEntry::zero(),
			page_fault: IdtEntry::zero(),
			_pad1: 0,
			x86_float_exception: IdtEntry::zero(),
			alignment_check: IdtEntry::zero(),
			machine_check: IdtEntry::zero(),
			simd_float_exception: IdtEntry::zero(),
			virtualization_exception: IdtEntry::zero(),
			control_protection_exception: IdtEntry::zero(),
			_pad2: [0; 6],
			hypervisor_injection_exception: IdtEntry::zero(),
			vmm_communication_exception: IdtEntry::zero(),
			security_exception: IdtEntry::zero(),
			_pad3: 0,
			others: [IdtEntry::zero(); 256 - 32],
		}
	}

	pub fn load(&self) {
		let idtr = Idtr {
			size: 0x1000 - 1,
			offset: self as _
		};
		unsafe {
			asm!("lidt [{}]", in(reg) &idtr);
		}
	}
}