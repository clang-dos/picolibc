#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2026 clang-dos contributors
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.


has_os_library = true
has_os_posix = true
has_os_fileio = true

srcs_os_win32 = [
  'clock.c',
  'ctors.c',
  'cxa_thread_atexit.c',
  'dirent.c',
  'error.c',
  'fd.c',
  'files.c',
  'keydtor.c',
  'lock.c',
  'lseek.c',
  'malloc.c',
  'open.c',
  'process.c',
  'read.c',
  'start.c',
  'stat.c',
  'thread.c',
  'time.c',
  'tlssup.c',
  'utime.c',
  'write.c',
]

srcs_os_win32_fallback = [
  'fpathconf.c',
  'kill.c',
  'pathconf.c',
  'raise.c',
  'signal.c',
  'sigprocmask.c',
  'sysconf.c',
]

srcs_libos += srcs_os_win32 + srcs_os_win32_fallback

src_os_win32 = files(srcs_os_win32)
foreach f : srcs_os_win32_fallback
  src_os_win32 += files('..' / 'fallback' / f)
endforeach

if really_install
  install_headers(['include/io.h', 'include/process.h'], install_dir: include_dir)
endif
