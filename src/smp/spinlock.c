#include "spinlock.h"

void spinlock_lock(spinlock* lock) {
	while (true) {
		if (!atomic_exchange_explicit(lock, true, memory_order_acquire)) break;
		while (atomic_load_explicit(lock, memory_order_relaxed)) __builtin_ia32_pause();
	}
}

bool spinlock_try_lock(spinlock* lock) {
	return !atomic_exchange_explicit(lock, true, memory_order_acquire);
}

void spinlock_unlock(spinlock* lock) {
	atomic_store_explicit(lock, false, memory_order_release);
}