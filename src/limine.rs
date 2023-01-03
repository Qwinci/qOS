#![allow(unused)]

use core::cell::UnsafeCell;
use core::ffi::c_char;
use core::ptr::NonNull;

#[repr(transparent)]
pub struct LiminePtr<T>(UnsafeCell<Option<NonNull<T>>>);
impl<T> LiminePtr<T> {
	const fn null() -> Self {
		Self (UnsafeCell::new(None))
	}
	pub fn get(&self) -> Option<&T> {
		unsafe {
			self.0.get().read_volatile().map(|p| p.as_ref())
		}
	}
}

#[repr(C, packed)]
struct Header {
	id: [u64; 4],
	revision: u64
}
impl Header {
	const fn new(id: [u64; 2]) -> Self {
		Self { id: [0xc7b1dd30df4c8b88, 0x0a82e883a194f07b, id[0], id[1]], revision: 0 }
	}
}

#[repr(C)]
pub struct FramebufferRequest {
	header: Header,
	pub response: LiminePtr<FramebufferResponse>
}
impl FramebufferRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0x9d5827dcd881dd75, 0xa3148604f6fab11b]),
		response: LiminePtr::null() }
	}
}
unsafe impl Sync for FramebufferRequest {}

#[repr(C)]
pub struct FramebufferResponse {
	revision: u64,
	pub framebuffer_count: u64,
	pub framebuffers: *const *mut Framebuffer
}

#[repr(C)]
pub struct Framebuffer {
	pub address: *mut u32,
	pub width: u64,
	pub height: u64,
	pub pitch: u64,
	pub bpp: u16,
	pub memory_model: u8,
	pub red_mask_size: u8,
	pub red_mask_shift: u8,
	pub green_mask_size: u8,
	pub green_mask_shift: u8,
	pub blue_mask_size: u8,
	pub blue_mask_shift: u8,
	unused: [u8; 7],
	pub edid_size: u64,
	pub edid: usize
}

#[repr(C)]
pub struct MemMapRequest {
	header: Header,
	pub response: LiminePtr<MemMapResponse>
}

impl MemMapRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0x67cf3d9d378a806f, 0xe304acdfc50c3c62]),
		response: LiminePtr::null() }
	}
}

unsafe impl Sync for MemMapRequest {}

#[repr(C)]
pub struct MemMapResponse {
	revision: u64,
	pub entry_count: u64,
	pub entries: *const *const MemMapEntry
}

#[repr(C)]
pub struct MemMapEntry {
	pub base: u64,
	pub length: u64,
	pub mem_type: MemType
}

#[repr(u64)]
#[derive(Copy, Clone, PartialEq, Debug)]
pub enum MemType {
	Usable,
	Reserved,
	AcpiReclaimable,
	AcpiNvs,
	BadMemory,
	BootloaderReclaimable,
	KernelAndModules,
	Framebuffer
}

#[repr(C)]
pub struct ModuleRequest {
	header: Header,
	pub response: LiminePtr<ModuleResponse>
}

impl ModuleRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0x3e7e279702be32af, 0xca1c4f3bd1280cee]),
		response: LiminePtr::null() }
	}
}

unsafe impl Sync for ModuleRequest {}

#[repr(C)]
pub struct ModuleResponse {
	revision: u64,
	pub module_count: u64,
	pub modules: *const *const File
}

#[repr(C)]
pub struct Uuid {
	pub a: u32,
	pub b: u16,
	pub c: u16,
	pub d: [u8; 8]
}

#[repr(C)]
pub struct File {
	revision: u64,
	pub address: usize,
	pub size: u64,
	pub path: *const c_char,
	pub cmdline: *const c_char,
	pub media_type: u32,
	unused: u32,
	pub tftp_ip: u32,
	pub tftp_port: u32,
	pub partition_index: u32,
	pub mbr_disk_id: u32,
	pub gpt_disk_uuid: Uuid,
	pub gpt_part_uuid: Uuid,
	pub part_uuid: Uuid
}

#[repr(C)]
pub struct RSDPRequest {
	header: Header,
	pub response: LiminePtr<RSDPResponse>
}

impl RSDPRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0xc5e77b6b397e7b43, 0x27637845accdcf3c]),
		response: LiminePtr::null() }
	}
}

unsafe impl Sync for RSDPRequest {}

#[repr(C)]
pub struct RSDPResponse {
	revision: u64,
	pub address: usize
}

#[repr(C)]
pub struct KernelAddressRequest {
	header: Header,
	pub response: LiminePtr<KernelAddressResponse>
}

impl KernelAddressRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0x71ba76863cc55f63, 0xb2644a48c516a487]),
		response: LiminePtr::null() }
	}
}

unsafe impl Sync for KernelAddressRequest {}

#[repr(C)]
pub struct KernelAddressResponse {
	revision: u64,
	pub physical_base: u64,
	pub virtual_base: u64
}

#[repr(C)]
pub struct HHDMRequest {
	header: Header,
	pub response: LiminePtr<HHDMResponse>
}

impl HHDMRequest {
	pub const fn new() -> Self {
		Self { header: Header::new([0x48dcf1cb8ad2b852, 0x63984e959a98244b]),
			response: LiminePtr::null() }
	}
}

unsafe impl Sync for HHDMRequest {}

#[repr(C)]
pub struct HHDMResponse {
	revision: u64,
	pub offset: u64
}