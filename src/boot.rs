#![allow(unused)]

struct Framebuffer {
	pub width: usize,
	pub height: usize,
	bpp: usize,
	pitch: usize,
	data: *mut u32
}

impl Framebuffer {
	const fn new(width: usize, height: usize, bpp: usize, pitch: usize, data: *mut u32) -> Self {
		Self { width, height, bpp, pitch, data }
	}

	fn write(&mut self, x: usize, y: usize, color: u32) {
		assert!(x < self.width && y < self.height);
		unsafe {
			self.data.add(y * self.pitch + x * self.bpp).write_volatile(color);
		}
	}
}