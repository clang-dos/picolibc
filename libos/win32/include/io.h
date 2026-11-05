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

/* The Windows C library declares setmode, the file mode flags and the
 * functions that go between descriptors and Win32 handles here. */

#ifndef _IO_H_
#define _IO_H_

#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#ifndef __win32_wchar
#ifdef __cplusplus
#define __win32_wchar wchar_t
#else
#define __win32_wchar __WCHAR_TYPE__
#endif
#endif

_BEGIN_STD_C

/* The Win32 handle of a descriptor, or -1. */
intptr_t _get_osfhandle(int fd);
/* A new descriptor for a Win32 handle; flags may hold O_TEXT, O_BINARY
 * and O_APPEND. Closing the descriptor closes the handle. */
int      _open_osfhandle(intptr_t handle, int flags);

int      _close(int fd);
int      _dup(int fd);
int      _dup2(int fd, int fd2);
int      _isatty(int fd);
/* Opens a file by its wide (UTF-16) name. */
int      _wopen(const __win32_wchar *path, int flags, ...);

_END_STD_C

#endif /* _IO_H_ */
