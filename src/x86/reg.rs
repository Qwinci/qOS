use core::arch::asm;
use core::marker::PhantomData;
use crate::paging::{PageTable, VirtAddr};

pub struct Cr3 {
	_phantom: PhantomData<PageTable>
}

impl Cr3 {
	pub fn read() -> &'static PageTable {
		let value: *const PageTable;
		unsafe {
			asm!("mov {}, cr3", out(reg) value, options(nomem, preserves_flags, nostack));
			&*value
		}
	}

	pub fn write(table: &'static PageTable) {
		let addr = VirtAddr::new(table as *const _ as usize).as_phys().as_usize();
		unsafe {
			asm!("mov cr3, {}", in(reg) addr, options(preserves_flags, nostack));
		}
	}
}