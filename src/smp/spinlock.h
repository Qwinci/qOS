#pragma once
#include <stdatomic.h>
#include <stdbool.h>

typedef atomic_bool spinlock;

void spinlock_lock(spinlock* lock);

bool spinlock_try_lock(spinlock* lock);

void spinlock_unlock(spinlock* lock);