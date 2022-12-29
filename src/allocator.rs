#![allow(unused)]

use core::alloc::{GlobalAlloc, Layout};
use core::cell::UnsafeCell;
use core::hint::unreachable_unchecked;
use core::ptr::NonNull;
use crate::allocator::page_allocator::PageAllocator;
use crate::limine::MemType;
use crate::start::MEM_MAP_REQUEST;
use crate::sync::Mutex;
use crate::utils::cold;

mod page_allocator {
	use core::ptr::NonNull;
	use crate::utils::{cold, HIGH_HALF_OFFSET};

	struct Node {
		size: usize,
		prev: Link,
		next: Link
	}

	type Link = Option<NonNull<Node>>;

	pub struct PageAllocator {
		root: Link,
		end: Link
	}

	impl PageAllocator {
		pub const fn new() -> Self {
			Self { root: None, end: None }
		}

		//noinspection DuplicatedCode
		unsafe fn try_merge_node(&self, node: NonNull<Node>) {
			let node_ptr = node.as_ptr();
			if let Some(next) = (*node_ptr).next {
				let next_ptr = next.as_ptr();
				if next_ptr as usize == node_ptr as usize + (*node_ptr).size * 0x1000 {
					(*node_ptr).size += (*next_ptr).size;
					(*node_ptr).next = (*next_ptr).next;
					if let Some(next) = (*next_ptr).next {
						(*next.as_ptr()).prev = Some(node);
					}
				}
			}
			if let Some(prev) = (*node_ptr).prev {
				let prev_ptr = prev.as_ptr();
				if prev_ptr as usize + (*prev_ptr).size * 0x1000 == node_ptr as usize {
					(*prev_ptr).size += (*node_ptr).size;
					(*prev_ptr).next = (*node_ptr).next;
					if let Some(next) = (*node_ptr).next {
						(*next.as_ptr()).prev = Some(prev);
					}
				}
			}
		}

		unsafe fn insert_node(&mut self, node: NonNull<Node>) {
			if self.root.is_none() {
				cold();
				self.root = Some(node);
				self.end = self.root;
				return;
			}

			let root = self.root.unwrap_unchecked();
			let end = self.end.unwrap_unchecked();

			let node_ptr = node.as_ptr();

			let mut n = root.as_ptr();

			if node_ptr < n {
				let old_root = self.root;
				self.root = Some(node);
				(*node_ptr).next = old_root;
				(*n).prev = Some(node);
				self.try_merge_node(node);
				return;
			}

			while let Some(next) = (*n).next {
				if node_ptr < next.as_ptr() {
					break;
				}

				n = next.as_ptr();
			}

			if let Some(next) = (*n).next {
				let next = next.as_ptr();
				(*next).prev = Some(node);
			}
			(*node_ptr).prev = Some(NonNull::new_unchecked(n));
			(*node_ptr).next = (*n).next;
			(*n).next = Some(node);
			if n == end.as_ptr() {
				self.end = Some(node);
			}
			self.try_merge_node(node);
		}

		unsafe fn remove_node(&mut self, node: NonNull<Node>) {
			let ptr = node.as_ptr();
			if let Some(prev) = (*ptr).prev {
				let prev = prev.as_ptr();
				(*prev).next = (*ptr).next;
			}
			if let Some(next) = (*ptr).next {
				let next = next.as_ptr();
				(*next).prev = (*ptr).prev;
			}
			if node == self.root.unwrap_unchecked() {
				self.root = (*ptr).next;
			}
			if node == self.end.unwrap_unchecked() {
				self.end = (*ptr).prev;
			}
		}

		unsafe fn get_min_size_min_addr(&mut self, count: usize) -> *mut u8 {
			let mut node = self.root;
			while let Some(n) = node {
				let ptr = n.as_ptr();

				if (*ptr).size >= count {
					let remaining = (*ptr).size - count;

					self.remove_node(n);

					if remaining > 0 {
						self.add_memory(ptr as usize + count * 0x1000, remaining * 0x1000);
					}

					return ptr as *mut u8;
				}

				node = (*ptr).next;
			}
			core::ptr::null_mut()
		}

		unsafe fn get_min_size(&mut self, count: usize) -> *mut u8 {
			let mut node = self.end;
			while let Some(n) = node {
				let ptr = n.as_ptr();

				if (*ptr).size >= count {
					let remaining = (*ptr).size - count;
					if remaining == 0 {
						self.remove_node(n);
						cold();
					}
					else {
						(*ptr).size = remaining;
					}
					return (ptr as usize + remaining * 0x1000) as *mut u8;
				}

				node = (*ptr).prev;
			}
			core::ptr::null_mut()
		}

		unsafe fn switch_mapping(&mut self) {
			if let Some(root) = self.root {
				self.root = Some(NonNull::new_unchecked(root.as_ptr().byte_add(HIGH_HALF_OFFSET)));
				self.end = Some(NonNull::new_unchecked(self.end.unwrap_unchecked().as_ptr().byte_add(HIGH_HALF_OFFSET)));

				let mut node = self.root;
				while let Some(n) = node {
					let n_ptr = n.as_ptr();
					if let Some(next) = (*n_ptr).next {
						(*n_ptr).next = Some(NonNull::new_unchecked(next.as_ptr().byte_add(HIGH_HALF_OFFSET)));
					}
					if let Some(prev) = (*n_ptr).prev {
						(*n_ptr).prev = Some(NonNull::new_unchecked(prev.as_ptr().byte_add(HIGH_HALF_OFFSET)));
					}
					node = (*n_ptr).next;
				}
			}
			else {
				cold();
			}
		}

		pub(crate) unsafe fn add_memory(&mut self, base: usize, size: usize) {
			let count = size / 0x1000;
			if count == 0 {
				return;
			}

			let node = base as *mut Node;
			node.write(Node { prev: None, next: None, size: count });
			self.insert_node(NonNull::new_unchecked(node));
		}

		pub fn free_pages(&mut self, pages: *mut u8, count: usize) {
			unsafe {
				self.add_memory(pages as usize, count * 0x1000)
			}
		}

		//noinspection DuplicatedCode
		#[must_use]
		pub fn alloc_pages(&mut self, pages: usize) -> *mut u8 {
			unsafe { self.get_min_size(pages) }
		}

		//noinspection DuplicatedCode
		#[must_use]
		pub fn alloc_pages_low(&mut self, pages: usize) -> *mut u8 {
			unsafe { self.get_min_size_min_addr(pages) }
		}
	}
}

const FREELIST_COUNT: usize = 9;

struct Node {
	next: Option<NonNull<Node>>
}

type Link = Option<NonNull<Node>>;

#[derive(Copy, Clone)]
struct Freelist {
	root: Link
}

pub struct Allocator<const LOW: bool = false> {
	freelists: UnsafeCell<[Freelist; FREELIST_COUNT]>
}

const fn size_to_index(size: usize) -> usize {
	if size <= 8 { 0 }
	else if size <= 16 { 1 }
	else if size <= 32 { 2 }
	else if size <= 64 { 3 }
	else if size <= 128 { 4 }
	else if size <= 256 { 5 }
	else if size <= 512 { 6 }
	else if size <= 1024 { 7 }
	else if size <= 2048 { 8 }
	else { cold(); 8 }
}

const fn index_to_size(index: usize) -> usize {
	match index {
		0 => 8,
		1 => 16,
		2 => 32,
		3 => 64,
		4 => 128,
		5 => 256,
		6 => 512,
		7 => 1024,
		8 => 2048,
		_ => unsafe { unreachable_unchecked() }
	}
}

impl<const LOW: bool> Allocator<LOW> {
	const fn new() -> Self {
		Self { freelists: UnsafeCell::new([Freelist {root: None}; FREELIST_COUNT]) }
	}

	unsafe fn try_merge_node(&self, index: usize, parent: NonNull<Node>, prev: Link) {
		let parent_ptr = parent.as_ptr();

		let inserted_node = (*parent_ptr).next.unwrap_unchecked();
		if index < FREELIST_COUNT - 1 {
			if parent_ptr as usize + index_to_size(index) == inserted_node.as_ptr() as usize {
				if let Some(prev) = prev {
					(*prev.as_ptr()).next = (*inserted_node.as_ptr()).next
				}
				self.freelist_insert(index + 1, parent_ptr as usize);
			}
		}
		else if index == FREELIST_COUNT - 1 {
			cold();
			if parent_ptr as usize + index_to_size(index) == inserted_node.as_ptr() as usize {
				if let Some(prev) = prev {
					(*prev.as_ptr()).next = (*inserted_node.as_ptr()).next
				}
				PAGE_ALLOCATOR.lock().free_pages(parent_ptr as _, 1);
			}
		}
	}

	unsafe fn freelist_insert(&self, index: usize, node: usize) {
		let list = (*self.freelists.get()).get_unchecked_mut(index);
		let node = node as *mut Node;
		node.write(Node { next: None });

		if let Some(root) = list.root {
			let mut n = root;
			let mut prev = None;
			while let Some(next) = (*n.as_ptr()).next {
				if n.as_ptr() >= node {
					(*n.as_ptr()).next = Some(NonNull::new_unchecked(node));
					(*node).next = Some(next);
					self.try_merge_node(index, n, prev);
					return;
				}

				prev = Some(n);
				n = next;
			}
			(*n.as_ptr()).next = Some(NonNull::new_unchecked(node));
		}
		else {
			list.root = Some(NonNull::new_unchecked(node));
		}
	}

	unsafe fn freelist_get(&self, index: usize) -> *mut u8 {
		let list = (*self.freelists.get()).get_unchecked_mut(index);

		if let Some(root) = list.root {
			list.root = (*root.as_ptr()).next;
			return root.as_ptr() as *mut u8;
		}
		else {
			if index == FREELIST_COUNT - 1 {
				let mem = if LOW {
					PAGE_ALLOCATOR.lock().alloc_pages_low(1)
				} else {
					PAGE_ALLOCATOR.lock().alloc_pages(1)
				};
				if mem.is_null() {
					return core::ptr::null_mut();
				}
				let node = mem.add(0x1000 / 2) as *mut Node;
				node.write(Node { next: None });
				list.root = Some(NonNull::new_unchecked(node));
				return mem;
			}
			else {
				let size = index_to_size(index);

				let orig_blocks = if LOW {
					PAGE_ALLOCATOR.lock().alloc_pages_low(1)
				} else {
					PAGE_ALLOCATOR.lock().alloc_pages(1)
				};
				if orig_blocks.is_null() {
					return core::ptr::null_mut();
				}
				let mut blocks = orig_blocks.add(size);
				let node = blocks as *mut Node;
				node.write(Node { next: None });
				list.root = Some(NonNull::new_unchecked(node));
				let remaining_blocks = 0x1000 / size - 2;
				let mut node = list.root.unwrap_unchecked();
				for _ in 0..remaining_blocks {
					let node_ptr = node.as_ptr();
					let new_node = blocks as *mut Node;
					new_node.write(Node { next: None });
					(*node_ptr).next = Some(NonNull::new_unchecked(new_node));
					blocks = blocks.add(size);
					node = NonNull::new_unchecked(new_node);
				}
				orig_blocks
			}
		}
	}

	fn alloc_size(&self, size: usize) -> *mut u8 {
		if size >= 0x1000 {
			let count = (size + 0x1000 - 1) / 0x1000;
			if LOW {
				PAGE_ALLOCATOR.lock().alloc_pages(count)
			} else {
				PAGE_ALLOCATOR.lock().alloc_pages_low(count)
			}
		}
		else {
			unsafe {
				self.freelist_get(size_to_index(size))
			}
		}
	}

	fn dealloc_size(&self, ptr: *mut u8, size: usize) {
		if size >= 0x1000 {
			let count = (size + 0x1000 - 1) / 0x1000;
			PAGE_ALLOCATOR.lock().free_pages(ptr, count);
		}
		else {
			unsafe {
				self.freelist_insert(size_to_index(size), ptr as usize);
			}
		}
	}
}

unsafe impl GlobalAlloc for Allocator {
	unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
		let aligned_layout = layout.pad_to_align();
		self.alloc_size(aligned_layout.size())
	}

	unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
		let aligned_layout = layout.pad_to_align();
		self.dealloc_size(ptr, aligned_layout.size())
	}
}

unsafe impl<const LOW: bool> Sync for Allocator<LOW> {}

#[global_allocator]
pub static ALLOCATOR: Allocator = Allocator::new();
pub static LOW_ALLOCATOR: Allocator<true> = Allocator::new();

pub static PAGE_ALLOCATOR: Mutex<PageAllocator> = Mutex::new(PageAllocator::new());

pub fn init_memory() {
	let memmap = MEM_MAP_REQUEST.response.get().unwrap();
	let mut allocator = PAGE_ALLOCATOR.lock();
	for i in 0..memmap.entry_count as usize {
		unsafe {
			let entry = &**memmap.entries.add(i);
			if entry.mem_type == MemType::Usable {
				allocator.add_memory(entry.base as usize, entry.length as usize);
			}
		}
	}
}