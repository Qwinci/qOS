#![allow(unused)]

use core::arch::x86_64::_mm_pause;
use core::cell::{LazyCell, UnsafeCell};
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{AtomicBool, Ordering};

#[must_use = "MutexGuard that is not used is immediately unlocked"]
pub struct MutexGuard<'a, T> {
	lock: &'a Mutex<T>
}

impl<'a, T> MutexGuard<'a, T> {
	fn new(lock: &'a Mutex<T>) -> Self {
		Self { lock }
	}
}

impl<'a, T> Deref for MutexGuard<'a, T> {
	type Target = T;

	fn deref(&self) -> &Self::Target {
		unsafe { &*self.lock.inner.get() }
	}
}

impl<'a, T> DerefMut for MutexGuard<'a, T> {
	fn deref_mut(&mut self) -> &mut Self::Target {
		unsafe { &mut *self.lock.inner.get() }
	}
}

unsafe impl<T: Sync> Sync for MutexGuard<'_, T> {}

impl<'a, T> Drop for MutexGuard<'a, T> {
	fn drop(&mut self) {
		self.lock.lock.store(false, Ordering::Release);
	}
}

pub struct Mutex<T> {
	lock: AtomicBool,
	inner: UnsafeCell<T>
}

pub type LockResult<'a, T> = Result<MutexGuard<'a, T>, ()>;

impl<T> Mutex<T> {
	pub const fn new(inner: T) -> Self {
		Self { lock: AtomicBool::new(false), inner: UnsafeCell::new(inner) }
	}

	pub fn lock(&self) -> MutexGuard<T> {
		loop {
			if self.lock.compare_exchange(false, true, Ordering::Acquire, Ordering::Acquire).is_ok() {
				break;
			}

			while self.lock.load(Ordering::Relaxed) {
				unsafe { _mm_pause() }
			}
		}
		MutexGuard::new(self)
	}

	pub fn try_lock(&self) -> LockResult<T> {
		if self.lock.compare_exchange(false, true, Ordering::Acquire, Ordering::Acquire).is_ok() {
			Ok(MutexGuard::new(self))
		}
		else {
			Err(())
		}
	}

	pub fn get(&mut self) -> &mut T {
		self.inner.get_mut()
	}
}

unsafe impl<T> Sync for Mutex<T> {}

pub struct LazyLock<T, F = fn() -> T> {
	lock: Mutex<()>,
	inner: LazyCell<T, F>
}

impl<T, F: FnOnce() -> T> LazyLock<T, F> {
	pub const fn new(init: F) -> Self {
		Self { lock: Mutex::new(()), inner: LazyCell::new(init) }
	}

	pub fn get(&self) -> &T {
		let _ = self.lock.lock();
		self.inner.deref()
	}
}

unsafe impl<T, F: FnOnce() -> T> Sync for LazyLock<T, F> {}