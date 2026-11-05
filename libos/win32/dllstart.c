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

/*
 * The entry point of DLLs. A DLL has its own copy of the library, with
 * its own descriptors and atexit list: it sets them up when the DLL is
 * loaded, runs its constructors, and runs its atexit functions and
 * destructors when it is unloaded. DllMain, if the DLL has one, is called
 * after the constructors and before the destructors.
 */

#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1

void __main(void);
void __call_exitprocs(int code, void *param);

extern __win32_bool __attribute__((__stdcall__, __weak__))
DllMain(__win32_handle module, __win32_dword reason, void *reserved);

__win32_bool __attribute__((__stdcall__)) DllMainCRTStartup(__win32_handle module, __win32_dword reason,
                                                            void *reserved);

__win32_bool __attribute__((__stdcall__, __used__))
DllMainCRTStartup(__win32_handle module, __win32_dword reason, void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH) {
        __win32_init();
        __main();
    }
    __win32_bool ret = DllMain ? DllMain(module, reason, reserved) : 1;
    if (reason == DLL_PROCESS_ATTACH && !ret) {
        /* Windows does not send DLL_PROCESS_DETACH after a failed attach. */
        __call_exitprocs(0, NULL);
    } else if (reason == DLL_PROCESS_DETACH) {
        __call_exitprocs(0, NULL);
    }
    return ret;
}
