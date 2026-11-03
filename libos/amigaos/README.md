# picolibc on AmigaOS

This directory has classic AmigaOS support (Kickstart 1.3 and later, any
68k CPU) for picolibc on the `m68k-amigaos` target of the
[clang-dos LLVM toolchain](https://github.com/clang-dos/llvm). It goes into
`libc.a` itself, with `crt0.o`, so a program needs nothing else from
picolibc. It uses no Amiga NDK headers: the few library offsets and
structure fields it needs are written out in `local-amigaos.h`.

## Building

With clang, `llvm-ar` and the other LLVM tools on `PATH`:

```sh
meson setup build-amigaos \
    --cross-file scripts/cross-clang-m68k-amigaos.txt \
    -Dos-amigaos=true -Denable-malloc=false -Dmultilib=false \
    -Dpicocrt=false -Dsemihost=false -Dtests=false -Dposix-console=true \
    -Dsingle-thread=true -Dthread-local-storage=false -Dspecsdir=none \
    --prefix=$PWD/install-amigaos
ninja -C build-amigaos install
```

Copy `include/` to `<sysroot>/include` and `lib/crt0.o` and `lib/libc.a`
to `<sysroot>/lib`. `clang --target=m68k-amigaos --sysroot=<sysroot>` then
links them with `libclang_rt.builtins.a` by default. The cross file builds
for the 68000, so the library runs on every Amiga.

## What it does

- `crt0.S` keeps the registers and stack pointer the program was started
  with, so that `_exit` can return to the CLI or Workbench from anywhere.
  From Workbench it waits for the startup message first.
- `start.c` opens dos.library (version 33, Kickstart 1.2, or later) and
  calls `main`. From the CLI, `argv` holds the words of the command line,
  split the way the CLI does (double quotes, `*"`, `*N` and `*E`), and
  `argv[0]` is the command name. From Workbench, `argc` is 0 and `argv` is
  the `struct WBStartup` message, as with other Amiga compilers, and the
  program runs in its own directory.
- `malloc` takes each block from `AllocMem()`, and `_exit` frees what is
  left, since AmigaOS doesn't free a program's memory. Build with
  `-Denable-malloc=false`.
- File descriptors 0, 1 and 2 are the CLI's input, output and error
  stream (the output on Kickstart 1.3). A Workbench program opens a
  console window the first time it uses them.
- Files: `open`, `read`, `write`, `lseek`, `close`, `stat`, `fstat`,
  `unlink`, `rename` (which replaces an existing file, as in POSIX),
  `mkdir`, `rmdir`, `chdir` and `getcwd` (Kickstart 2.0 and later),
  `access`, `isatty`. Errors from `IoErr()` become `errno` values.
- Time comes from `DateStamp()`, with local time taken as UTC and a
  resolution of 1/50 second. `nanosleep` calls `Delay()`.
- Signals are those of the fallback library: `raise` works, Ctrl-C is not
  checked.

The CLI gives a program the stack size set with `Stack` (4000 bytes by
default on Kickstart 1.3), which is small for `printf` with floating
point; set 16000 or more.
