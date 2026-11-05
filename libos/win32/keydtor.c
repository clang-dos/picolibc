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
#include <stdlib.h>
#include <sys/lock.h>

/*
 * Destructors for TlsAlloc keys, run when a thread ends. Windows before
 * Server 2003 has no fiber local storage to do this, so libc++ registers
 * its keys here, through the same entry point the mingw-w64 runtime
 * offers. The destructors run from a TLS callback, which this file brings
 * in along with the functions.
 */

struct key_dtor {
    __win32_dword    key;
    void             (*dtor)(void *);
    struct key_dtor *next;
};

static struct key_dtor *key_dtors;

int                     ___w64_mingwthr_add_key_dtor(__win32_dword key, void (*dtor)(void *));
int                     ___w64_mingwthr_remove_key_dtor(__win32_dword key);

int
___w64_mingwthr_add_key_dtor(__win32_dword key, void (*dtor)(void *))
{
    struct key_dtor *d = malloc(sizeof(*d));
    if (!d)
        return -1;
    d->key = key;
    d->dtor = dtor;
    __LIBC_LOCK();
    d->next = key_dtors;
    key_dtors = d;
    __LIBC_UNLOCK();
    return 0;
}

int
___w64_mingwthr_remove_key_dtor(__win32_dword key)
{
    struct key_dtor *found = NULL;
    __LIBC_LOCK();
    for (struct key_dtor **p = &key_dtors; *p; p = &(*p)->next) {
        if ((*p)->key == key) {
            found = *p;
            *p = found->next;
            break;
        }
    }
    __LIBC_UNLOCK();
    /* A thread that is ending may still be walking the list, so the entry
     * is not freed. */
    (void)found;
    return 0;
}

#define DLL_PROCESS_DETACH 0
#define DLL_THREAD_DETACH  3

static void __attribute__((__stdcall__))
run_key_dtors(void *module, __win32_dword reason, void *reserved)
{
    (void)module;
    (void)reserved;
    if (reason != DLL_THREAD_DETACH && reason != DLL_PROCESS_DETACH)
        return;
    /* Keys are only added at the head, so the list can be walked without
     * the lock once the head has been read. */
    __LIBC_LOCK();
    struct key_dtor *d = key_dtors;
    __LIBC_UNLOCK();
    for (; d; d = d->next) {
        void *value = TlsGetValue(d->key);
        if (value)
            d->dtor(value);
    }
}

static void(__attribute__((__stdcall__)) * const key_dtor_callback)(void *, __win32_dword, void *)
    __attribute__((__section__(".CRT$XLD"), __used__))
    = run_key_dtors;
