/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 clang-dos contributors
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "local-win32.h"
#include <sys/lock.h>

#ifndef __SINGLE_THREAD

/* Locks are critical sections, which every Windows has. A static lock
 * starts zeroed and sets itself up the first time it is taken; state goes
 * from 0 to 1 while one thread does that, then to 2. */
struct __lock {
    struct __win32_critical_section cs;
    volatile long                   state;
};

struct __lock __lock___libc_recursive_mutex;

static void
setup(struct __lock *lock)
{
    for (;;) {
        long state = __atomic_load_n(&lock->state, __ATOMIC_ACQUIRE);
        if (state == 2)
            return;
        if (state == 0 && __atomic_exchange_n(&lock->state, 1, __ATOMIC_ACQUIRE) == 0) {
            InitializeCriticalSection(&lock->cs);
            __atomic_store_n(&lock->state, 2, __ATOMIC_RELEASE);
            return;
        }
        /* Another thread is setting it up. */
        Sleep(0);
    }
}

void
__retarget_lock_init(_LOCK_T *lock)
{
    struct __lock *l = HeapAlloc(GetProcessHeap(), __WIN32_HEAP_ZERO_MEMORY, sizeof(*l));
    if (l) {
        InitializeCriticalSection(&l->cs);
        l->state = 2;
    }
    *lock = l;
}

void
__retarget_lock_init_recursive(_LOCK_T *lock)
{
    __retarget_lock_init(lock);
}

void
__retarget_lock_close(_LOCK_T lock)
{
    if (lock && lock != &__lock___libc_recursive_mutex) {
        if (lock->state == 2)
            DeleteCriticalSection(&lock->cs);
        HeapFree(GetProcessHeap(), 0, lock);
    }
}

void
__retarget_lock_close_recursive(_LOCK_T lock)
{
    __retarget_lock_close(lock);
}

void __no_thread_safety_analysis
__retarget_lock_acquire(_LOCK_T lock)
{
    if (!lock)
        return;
    setup(lock);
    EnterCriticalSection(&lock->cs);
}

void __no_thread_safety_analysis
__retarget_lock_acquire_recursive(_LOCK_T lock)
{
    __retarget_lock_acquire(lock);
}

void __no_thread_safety_analysis
__retarget_lock_release(_LOCK_T lock)
{
    if (lock)
        LeaveCriticalSection(&lock->cs);
}

void __no_thread_safety_analysis
__retarget_lock_release_recursive(_LOCK_T lock)
{
    __retarget_lock_release(lock);
}

#endif /* __SINGLE_THREAD */
