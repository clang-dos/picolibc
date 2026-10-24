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

# DOS (INT 21h) system calls and startup code for i8086. These go into
# libc itself, so a program links with crt0.obj and libc only.

has_os_library = true
has_os_posix = true
has_os_fileio = true

srcs_os_msdos = [
  '_exit.c',
  'access.c',
  'chdir.c',
  'clock_getres.c',
  'clock_gettime.c',
  'close.c',
  'dup.c',
  'dup2.c',
  'error.c',
  'fdflags.c',
  'execve.c',
  'fork.c',
  'fstat.c',
  'getcwd.c',
  'getentropy.c',
  'getpid.c',
  'gettimeofday.c',
  'getuid.c',
  'halloc.c',
  'int21.S',
  'isatty.c',
  'isdevice.c',
  'lseek.c',
  'mkdir.c',
  'dosmktime.c',
  'nanosleep.c',
  'open.c',
  'pipe.c',
  'read.c',
  'rename.c',
  'rmdir.c',
  'sbrk.c',
  'setmode.c',
  'start.c',
  'stat.c',
  'tcgetattr.c',
  'tcsetattr.c',
  'dostime.c',
  'times.c',
  'unlink.c',
  'waitpid.c',
  'wep.c',
  'win16.c',
  'winmain.c',
  'libmain.c',
  'write.c',
]

# Signals and sysconf come from the fallback library.
srcs_os_msdos_fallback = [
  'fpathconf.c',
  'kill.c',
  'pathconf.c',
  'raise.c',
  'signal.c',
  'sigprocmask.c',
  'sysconf.c',
]

srcs_libos += srcs_os_msdos + srcs_os_msdos_fallback

src_os_msdos = files(srcs_os_msdos)
foreach f : srcs_os_msdos_fallback
  src_os_msdos += files('..' / 'fallback' / f)
endforeach

# crt0.obj for each target. Linkers for OMF cannot combine objects, so
# this is a plain object rather than a linked executable.
foreach params : targets
  target = params['name']
  target_dir = params['dir']
  target_c_args = params['c_args']

  instdir = join_paths(lib_dir, target_dir)

  custom_target('crt0' + target,
                input : 'crt0.S',
                output : 'crt0.obj',
                command : cc.cmd_array() + target_c_args + c_args +
                          ['-c', '@INPUT@', '-o', '@OUTPUT@'],
                install : really_install,
                install_dir : instdir,
                build_by_default : true)

  # The startup code of 16-bit Windows DLLs.
  if cc.get_define('__WINDOWS__', args : target_c_args + c_args) != ''
    custom_target('libentry' + target,
                  input : 'libentry.S',
                  output : 'libentry.obj',
                  command : cc.cmd_array() + target_c_args + c_args +
                            ['-c', '@INPUT@', '-o', '@OUTPUT@'],
                  install : really_install,
                  install_dir : instdir,
                  build_by_default : true)
  endif
endforeach
