# picolibc on DOS

This directory has DOS support for picolibc on the i8086 target of the
[clang-dos LLVM toolchain](https://github.com/clang-dos/llvm). It is built
into `libc.a` itself, together with `crt0.obj`, so a program needs
nothing else from picolibc.

## Building

One build per memory model (`tiny` makes .COM programs, `small` makes
.EXE programs), with clang, `llvm-ar` and a Watcom-style linker (wlink or
JWlink) on `PATH`:

```sh
meson setup build-small \
    --cross-file scripts/cross-clang-i8086-msdos-small.txt \
    -Dos-msdos=true -Dmultilib=false -Dpicocrt=false -Dsemihost=false \
    -Dtests=false -Dposix-console=true -Dsingle-thread=true \
    -Dthread-local-storage=false -Dspecsdir=none \
    --prefix=$PWD/install-small
ninja -C build-small install
```

Copy `include/` to `<sysroot>/include` and `lib/crt0.obj` and `lib/libc.a`
to `<sysroot>/lib/small`. `clang --target=i8086-pc-msdos --sysroot=<sysroot>`
then links them, with `libclang_rt.builtins.a`, by default.

## What the startup code does

`crt0.S` sets DS, ES and SS to DGROUP (an .EXE starts with SS at its
STACK segment), clears BSS and calls `__msdos_start` in `start.c`, which:

- sets up the heap. In an .EXE it is the rest of DGROUP after the stack,
  up to 64 KiB or the end of the program's memory. In a .COM it ends
  2 KiB below the stack, which DOS puts at the top of the segment.
- splits the command tail at PSP:80h into `argv`. Double quotes group
  words, and `\"` is a quote, as in the Microsoft C runtime.
- copies the environment block into `environ` and takes `argv[0]` from
  the program path after it (DOS 3 and later).
- runs the constructors (the XI segment) and calls `exit(main(...))`.
  `exit` runs `atexit` handlers and destructors (the YI segment).

The stack of an .EXE is 4 KiB; build with
`-D__MSDOS_STACK_SIZE=<bytes>` in `c_args` to change it.

## System calls

File I/O, `stat`, directories, `rename`, `dup`/`dup2`, `isatty`, the clock
(`time`, `gettimeofday`, `clock_gettime`, `times`, `nanosleep`) and `_exit`
use INT 21h. DOS error codes become `errno` values in `error.c`.

- Files are binary: there is no text-mode translation. Writes to a
  character device (the console, a printer) turn LF into CR LF, and reads
  from one drop CR.
- `O_APPEND` seeks to the end before each write.
- The DOS clock is local time; it is treated as UTC. It ticks about 18
  times a second.
- `fork`, `execve`, `pipe`, `waitpid` and `getentropy` fail with
  `ENOSYS` (`waitpid` with `ECHILD`); terminal attributes with `ENOTTY`.
  Without `getentropy`, `arc4random` runs from a fixed all-zero seed, so
  its output is predictable. A program can define `arc4random_abort` to
  be called instead.
- Signals, `raise` and `sysconf` come from `libos/fallback`.
