#
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright © 2026 clang-dos contributors
#
# OS/2 system calls (DOSCALLS), sbrk and startup code, for 16-bit OS/2 1.x
# (i8086, OMF objects, crt0.obj) and 32-bit OS/2 2.0 and later (i386, ELF
# objects, crt0.o). These go into libc itself.
#
has_os_library = true
has_os_posix = true
has_os_fileio = true

srcs_os_os2 = [
  '_exit.c',
  'error.c',
  'fd.c',
  'files.c',
  'read.c',
  'sbrk.c',
  'start.c',
  'time.c',
  'write.c',
]

srcs_os_os2_fallback = [
  'fpathconf.c',
  'getpid.c',
  'kill.c',
  'pathconf.c',
  'raise.c',
  'signal.c',
  'sigprocmask.c',
  'sysconf.c',
]

srcs_libos += srcs_os_os2 + srcs_os_os2_fallback

src_os_os2 = files(srcs_os_os2)

# On i8086 the compiler calls the integer (__i_printf) and long
# (__l_printf) variants of printf and scanf when the format allows; they
# are in the DOS layer's iovariant directory.
if host_machine.cpu_family() == 'i8086'
  foreach func : ['printf', 'fprintf', 'sprintf', 'snprintf', 'asprintf',
                  'scanf', 'fscanf', 'sscanf']
    foreach variant : ['i', 'l']
      src_os_os2 += files('..' / 'msdos' / 'iovariant' / func + variant + '.c')
    endforeach
  endforeach
endif
foreach f : srcs_os_os2_fallback
  src_os_os2 += files('..' / 'fallback' / f)
endforeach

# OMF objects are .obj, as the Watcom linkers expect.
crt0_name = host_machine.cpu_family() == 'i8086' ? 'crt0.obj' : 'crt0.o'

foreach params : targets
  target = params['name']
  target_dir = params['dir']
  target_c_args = params['c_args']

  instdir = join_paths(lib_dir, target_dir)

  custom_target('crt0' + target,
                input : 'crt0.S',
                output : crt0_name,
                command : cc.cmd_array() + target_c_args + c_args +
                          ['-c', '@INPUT@', '-o', '@OUTPUT@'],
                install : really_install,
                install_dir : instdir,
                build_by_default : true)

  # The startup code of 32-bit DLLs.
  if host_machine.cpu_family() != 'i8086'
    custom_target('dllcrt0' + target,
                  input : 'dllcrt0.S',
                  output : 'dllcrt0.o',
                  command : cc.cmd_array() + target_c_args + c_args +
                            ['-c', '@INPUT@', '-o', '@OUTPUT@'],
                  install : really_install,
                  install_dir : instdir,
                  build_by_default : true)
  endif
endforeach
