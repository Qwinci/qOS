#![allow(unused)]

use core::ffi::CStr;
use core::fmt;
use core::fmt::{Error, Write};
use crate::start::{FONT, FRAMEBUFFER_REQUEST, MODULE_REQUEST};
use crate::sync::{LazyLock, Mutex};

pub struct Framebuffer {
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

	pub fn write(&mut self, x: usize, y: usize, color: u32) {
		assert!(x < self.width && y < self.height);
		unsafe {
			self.data.byte_add(y * self.pitch + x * self.bpp).write_volatile(color);
		}
	}

	pub fn clear(&mut self, color: u32) {
		for y in 0..self.height {
			for x in 0..self.width {
				self.write(x, y, color);
			}
		}
	}
}

struct PsfFont {
	header_size: u32,
	flags: u32,
	num_glyph: u32,
	bytes_per_glyph: u32,
	width: u32,
	height: u32,
	data: *const u8
}

impl PsfFont {
	const MAGIC: u32 = 0x864ab572;
}

struct Writer {
	fb: Framebuffer,
	font: PsfFont,
	line: usize,
	col: usize,
	fg: u32,
	bg: u32
}

impl Writer {
	fn new(fb: Framebuffer, font: PsfFont) -> Self {
		Self { fb, font, line: 0, col: 0, fg: 0x00FF00, bg: 0 }
	}

	pub fn set_fg(&mut self, color: u32) {
		self.fg = color;
	}

	pub fn set_bg(&mut self, color: u32) {
		self.bg = color;
	}

	pub fn clear(&mut self) {
		self.line = 0;
		self.col = 0;
		self.fb.clear(self.bg);
	}
}

impl Write for Writer {
	fn write_str(&mut self, s: &str) -> fmt::Result {
		if self.col * self.font.width as usize >= self.fb.width {
			self.line += 1;
			self.col = 0;
		}
		if self.line * self.font.height as usize >= self.fb.height {
			return Err(Error);
		}

		let data = unsafe { self.font.data.add(self.font.header_size as usize) };
		for char in s.chars() {
			if char == '\n' {
				self.line += 1;
				self.col = 0;
				continue;
			}
			else if char == '\t' {
				for _ in 0..(4 - self.col % 4) {
					self.write_char(' ')?;
				}
				continue;
			}

			if self.col * self.font.width as usize >= self.fb.width {
				self.line += 1;
				self.col = 0;
			}
			if self.line * self.font.height as usize >= self.fb.height {
				return Err(Error);
			}

			let char_data = unsafe { data.add(char as usize * self.font.bytes_per_glyph as usize) };
			for y in 0..self.font.height as usize {
				let line = unsafe { *char_data.add(y) };
				for x in 0..self.font.width as usize {
					let color = if line & 1 << (7 - x) > 0 { self.fg } else { self.bg };
					self.fb.write(self.col * self.font.width as usize + x, self.line * self.font.height as usize + y, color);
				}
			}
			self.col += 1;
		}
		Ok(())
	}
}

static WRITER: LazyLock<Mutex<Writer>> = LazyLock::new(|| {
	let fbs = FRAMEBUFFER_REQUEST.response.get().unwrap();
	let fb = unsafe { &**fbs.framebuffers };

	let response = MODULE_REQUEST.response.get().unwrap();
	let mut font = None;
	for i in 0..response.module_count as usize {
		let module = unsafe { &**response.modules.add(i) };
		let str = unsafe { CStr::from_ptr(module.cmdline) }.to_str().unwrap();
		if str == FONT {
			let ptr = module.address as *const u32;
			let magic = unsafe { *ptr };
			if magic != PsfFont::MAGIC {
				break;
			}
			let header_size = unsafe { *ptr.add(2) };
			let flags = unsafe { *ptr.add(3) };
			let num_glyph = unsafe { *ptr.add(4) };
			let bytes_per_glyph = unsafe { *ptr.add(5) };
			let height = unsafe { *ptr.add(6) };
			let width = unsafe { *ptr.add(7) };

			font = Some(PsfFont {
				header_size,
				flags,
				num_glyph,
				bytes_per_glyph,
				width,
				height,
				data: ptr as *const u8
			})
		}
	}

	let font = font.unwrap();

	Mutex::new(Writer::new(
		Framebuffer::new(
			fb.width as usize,
			fb.height as usize,
			fb.bpp as usize / 8,
			fb.pitch as usize,
			fb.address),
		font))
});

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
	    $crate::fb::_print(format_args!($($arg)*));
    }};
}

#[macro_export]
macro_rules! println {
    () => {
	    $crate::print!("\n")
    };
	($($arg:tt)*) => {{
		$crate::print!("{}\n", format_args!($($arg)*));
	}};
}

pub fn _print(args: fmt::Arguments) {
	WRITER.get().lock().write_fmt(args).unwrap();
}