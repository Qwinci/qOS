#![feature(pointer_byte_offsets)]
#![feature(once_cell)]
#![cfg_attr(not(test), no_std)]
#![cfg_attr(not(test), no_main)]

extern crate alloc;

mod limine;
mod sync;
mod fb;
mod start;
mod boot;
mod allocator;
mod utils;

pub fn main() {

}