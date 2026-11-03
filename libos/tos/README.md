# picolibc on Atari TOS

This directory has Atari TOS support (TOS 1.0 and later, and EmuTOS, on
any 68k CPU) for picolibc on the `m68k-atari-tos` target of the
[clang-dos LLVM toolchain](https://github.com/clang-dos/llvm). It goes into
`libc.a` itself, with `crt0.o` and the linker script `tos.ld`, so a
program needs nothing else from picolibc. It uses no Atari headers: the
GEMDOS and XBIOS calls it makes are written out in `local-tos.h`.

## Building

With clang, `llvm-ar` and the other LLVM tools on `PATH`:

```sh
meson setup build-tos \
    --cross-file scripts/cross-clang-m68k-atari-tos.txt \
    -Dos-tos=true -Dmultilib=false -Dpicocrt=false -Dsemihost=false \
    -Dtests=false -Dposix-console=true -Dsingle-thread=true \
    -Dthread-local-storage=false -Dspecsdir=none \
    --prefix=$PWD/install-tos
ninja -C build-tos install
```

Copy `include/` to `<sysroot>/include` and `lib/crt0.o`, `lib/tos.ld`,
`lib/libc.a` and `lib/libm.a` to `<sysroot>/lib`.
`clang --target=m68k-atari-tos --sysroot=<sysroot>` then links them with
`libclang_rt.builtins.a` by default. The cross file builds for the 68000,
so the library runs on every Atari.

## What it does

- `crt0.S` gets the basepage, keeps the program's text, data, bss and a
  stack of `_stksize` bytes (32K unless the program defines it), and gives
  the rest of memory back with `Mshrink()`.
- `start.c` makes `environ` from the GEMDOS environment and `argv` from
  the command line in the basepage, or from the `ARGV` environment
  convention when the caller uses it, and calls `main`.
- `sbrk` hands out one block from `Malloc()`, taken on first use, for
  picolibc's `malloc`, which needs its memory in one piece: all of the
  largest free block but 64K, or `_heapsize` bytes when the program
  defines it.
- File descriptors 0 and 1 are GEMDOS handles 0 and 1; fd 2 is the
  console device (handle -1), since GEMDOS handle 2 is the serial port.
  Writing `\n` to a character device writes `\r\n`, and a `\r` read from
  one becomes `\n`.
- Files: `open`, `read`, `write`, `lseek`, `close`, `stat`, `fstat`,
  `unlink`, `rename` (which replaces an existing file, as in POSIX),
  `mkdir`, `rmdir`, `chdir` (with a drive letter, it also changes the
  drive), `getcwd`, `access`, `isatty`, `dup`, `dup2`. `/` in paths is
  taken as `\`. Errors become `errno` values.
- Time: GEMDOS local time, taken as UTC, with the 200 Hz timer for the
  fraction of a second and for `CLOCK_MONOTONIC`, `times` and
  `nanosleep`.
- Signals are those of the fallback library: `raise` works.
