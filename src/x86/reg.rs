use core::arch::asm;
use core::marker::PhantomData;
use crate::{bitflags, println};
use crate::paging::{PageTable, VirtAddr};

pub struct Cr3 {
	_phantom: PhantomData<PageTable>
}

impl Cr3 {
	pub fn read() -> &'static mut PageTable {
		let value: *mut PageTable;
		unsafe {
			asm!("mov {}, cr3", out(reg) value, options(nomem, preserves_flags, nostack));
			&mut *value
		}
	}

	pub fn write(table: &'static PageTable) {
		let virt = VirtAddr::new(table as *const _ as usize);
		let addr = VirtAddr::new(table as *const _ as usize).as_phys().as_usize();
		unsafe {
			println!("pml4 addr: {:#X}", addr);
			asm!("mov cr3, {}", in(reg) addr, options(preserves_flags, nostack));
		}
	}
}

pub struct Cr4 {
	_phantom: ()
}

bitflags! {
	pub struct Cr4Flags: u64 {
		vme = bool, 0..1;
		pvi = bool, 1..2;
		tsd = bool, 2..3;
		de = bool, 3..4;
		pse = bool, 4..5;
		pae = bool, 5..6;
		mce = bool, 6..7;
		pge = bool, 7..8;
		pce = bool, 8..9;
		osfxsr = bool, 9..10;
		osxmmexcpt = bool, 10..11;
		umip = bool, 11..12;
		vmxe = bool, 13..14;
		smxe = bool, 14..15;
		fsgsbase = bool, 16..17;
		pcide = bool, 17..18;
		osxsave = bool, 18..19;
		smep = bool, 20..21;
		smap = bool, 21..22;
		pke = bool, 22..23;
		cet = bool, 23..24;
		pks = bool, 24..45;
	}
}

impl Cr4 {
	pub fn read() -> Cr4Flags {
		let flags: u64;
		unsafe {
			asm!("mov {}, cr4", out(reg) flags);
		}
		Cr4Flags::from(flags)
	}

	pub fn write(flags: Cr4Flags) {
		let flags = flags.into_inner();
		unsafe {
			asm!("mov cr4, {}", in(reg) flags);
		}
	}
}