#![allow(unused_variables, unused_assignments, dead_code)]

use alloc::boxed::Box;
use core::fmt::{Debug, Formatter};
use crate::{bitflags, print, println};
use crate::utils::{get_high_half_offset, HIGH_HALF_OFFSET};

bitflags! {
	#[derive(Copy, Clone, PartialEq)]
	pub struct Flags: u64 {
		present = bool, 0..1;
		rw = bool, 1..2;
		user = bool, 2..3;
		write_through = bool, 3..4;
		cache_disable = bool, 4..5;
		accessed = bool, 5..6;
		dirty = bool, 6..7;
		huge = bool, 7..8;
		global = bool, 8..9;
		nx = bool, 63..64;
	}
}

impl Debug for Flags {
	fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
		write!(f, "Flags {{ present: {}, rw: {}, user: {}, write_through: {}, \
		cache_disable: {}, accessed: {}, dirty: {}, huge: {}, global: {}, nx: {} }}",
		self.get_present(), self.get_rw(), self.get_user(), self.get_write_through(),
		self.get_cache_disable(), self.get_accessed(), self.get_dirty(),
		self.get_huge(), self.get_global(), self.get_nx())
	}
}

#[repr(align(0x1000))]
pub struct PageTable {
	entries: [PageTableEntry; 512]
}

#[repr(transparent)]
#[derive(Copy, Clone, PartialEq, PartialOrd, Hash, Ord, Eq, Default)]
pub struct VirtAddr {
	addr: usize
}

impl Debug for VirtAddr {
	fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
		write!(f, "{:#X}", self.addr)
	}
}

impl VirtAddr {
	pub const fn new(addr: usize) -> Self {
		Self { addr }
	}

	pub const fn as_usize(&self) -> usize {
		self.addr
	}

	pub fn as_phys(&self) -> PhysAddr {
		PhysAddr::from(*self)
	}

	pub fn offset(&mut self, offset: isize) {
		self.addr = self.addr.checked_add_signed(offset).unwrap()
	}
}

impl From<PhysAddr> for VirtAddr {
	fn from(value: PhysAddr) -> Self {
		Self::new(value.addr + get_high_half_offset())
	}
}

#[repr(transparent)]
#[derive(Copy, Clone, PartialEq, PartialOrd, Hash, Ord, Eq, Default)]
pub struct PhysAddr {
	addr: usize
}

impl Debug for PhysAddr {
	fn fmt(&self, f: &mut Formatter<'_>) -> core::fmt::Result {
		write!(f, "{:#X}", self.addr)
	}
}

impl PhysAddr {
	pub const fn new(addr: usize) -> Self {
		Self { addr }
	}

	pub const fn as_usize(&self) -> usize {
		self.addr
	}

	pub fn as_virt(&self) -> VirtAddr {
		VirtAddr::from(*self)
	}

	pub const fn offset(&self, offset: isize) -> Self {
		Self::new(match self.addr.checked_add_signed(offset) {
			Some(addr) => addr,
			None => panic!("attempted to offset with overflow")
		})
	}
}

impl From<VirtAddr> for PhysAddr {
	fn from(value: VirtAddr) -> Self {
		Self::new(value.addr - get_high_half_offset())
	}
}

#[repr(align(0x1000))]
struct AlignedFrame {
	entries: [PageTableEntry; 512]
}

impl PageTable {
	pub const fn new() -> Self {
		Self { entries: [PageTableEntry::new(); 512] }
	}

	pub fn map(&mut self, phys: PhysAddr, virt: VirtAddr, mut flags: Flags) {
		let mut phys_addr = phys.as_usize();
		let mut virt_addr = virt.as_usize();

		let huge = flags.get_huge();

		if huge {
			assert_eq!(phys_addr & (0x200000 - 1), 0);
			assert_eq!(virt_addr & (0x200000 - 1), 0);
		}
		else {
			assert_eq!(phys_addr & (0x1000 - 1), 0);
			assert_eq!(virt_addr & (0x1000 - 1), 0);
		}

		flags.set_present(true);
		flags.set_huge(false);

		virt_addr >>= 12;
		let pt_offset = virt_addr & 0x1FF;
		virt_addr >>= 9;
		let pd_offset = virt_addr & 0x1FF;
		virt_addr >>= 9;
		let pdp_offset = virt_addr & 0x1FF;
		virt_addr >>= 9;
		let pml4_offset = virt_addr & 0x1FF;

		let pdp_entry = if self.entries[pml4_offset].get_flags().get_present() {
			unsafe {
				core::mem::transmute(self.entries[pml4_offset].get_addr().as_virt().as_usize())
			}
		} else {
			let frame = Box::new(AlignedFrame { entries: [PageTableEntry::new(); 512] });
			let frame = Box::leak(frame);
			let pml4_entry = &mut self.entries[pml4_offset];
			pml4_entry.set_flags(flags);
			pml4_entry.set_addr(VirtAddr::new(frame as *mut _ as usize).as_phys());
			frame
		};

		let pd_entry = if pdp_entry.entries[pdp_offset].get_flags().get_present() {
			unsafe {
				core::mem::transmute(pdp_entry.entries[pdp_offset].get_addr().as_virt().as_usize())
			}
		} else {
			let frame = Box::new(AlignedFrame { entries: [PageTableEntry::new(); 512] });
			let frame = Box::leak(frame);
			let pdp_entry = &mut pdp_entry.entries[pdp_offset];
			pdp_entry.set_flags(flags);
			pdp_entry.set_addr(VirtAddr::new(frame as *mut _ as usize).as_phys());
			frame
		};

		if huge {
			flags.set_huge(true);
			pd_entry.entries[pd_offset].set_flags(flags);
			pd_entry.entries[pd_offset].set_addr(phys);
			return;
		}

		let pt_entry = if pd_entry.entries[pd_offset].get_flags().get_present() {
			unsafe {
				core::mem::transmute(pd_entry.entries[pd_offset].get_addr().as_virt().as_usize())
			}
		} else {
			let frame = Box::new(AlignedFrame { entries: [PageTableEntry::new(); 512] });
			let frame = Box::leak(frame);
			let pd_entry = &mut pd_entry.entries[pd_offset];
			pd_entry.set_flags(flags);
			pd_entry.set_addr(VirtAddr::new(frame as *mut _ as usize).as_phys());
			frame
		};

		pt_entry.entries[pt_offset].set_flags(flags);
		pt_entry.entries[pt_offset].set_addr(phys);
	}
}

#[derive(Copy, Clone)]
#[repr(transparent)]
struct PageTableEntry {
	value: u64
}

impl PageTableEntry {
	const fn new() -> Self {
		Self { value: 0 }
	}

	const fn get_addr(&self) -> PhysAddr {
		PhysAddr::new((self.value & 0x000FFFFFFFFFFFF000) as usize)
	}

	fn set_addr(&mut self, addr: PhysAddr) {
		self.value |= addr.as_usize() as u64;
	}

	const fn get_flags(&self) -> Flags {
		Flags::from_bits_truncated(self.value)
	}

	fn set_flags(&mut self, flags: Flags) {
		self.value |= flags.into_inner();
	}
}
