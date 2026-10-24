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
STACK segment), clears BSS (and with far data pointers the far BSS after
the stack) and calls `__msdos_start` in `start.c`, which:

- sets up the heap. In a small or medium model .EXE it is the rest of
  DGROUP after the stack, up to 64 KiB or the end of the program's memory.
  In compact and large it is a segment of its own after the program (after
  far BSS), up to 64 KiB; in huge it runs from there to the end of the
  program's memory. In a .COM it ends 2 KiB below the stack, which DOS
  puts at the top of the segment.
- splits the command tail at PSP:80h into `argv`. Double quotes group
  words, and `\"` is a quote, as in the Microsoft C runtime.
- copies the environment block into `environ` and takes `argv[0]` from
  the program path after it (DOS 3 and later).
- runs the constructors (the XI segment) and calls `exit(main(...))`.
  `exit` runs `atexit` handlers and destructors (the YI segment).

The stack of an .EXE is 4 KiB; build with
`-D__MSDOS_STACK_SIZE=<bytes>` in `c_args` to change it, or link a
program with the linker's `option stack=<bytes>`, which makes the STACK
segment larger. In the small and medium models crt0 records the top of the
stack in `__msdos_stack_top`, and the heap starts there when that is after
`__heap_start`.

In every model but tiny, DGROUP starts with 16 bytes that nothing uses
(in Windows, the bytes that Windows keeps), so that no variable or string
is at offset 0, where a near pointer to it would be null.

## System calls

File I/O, `stat`, directories, `rename`, `dup`/`dup2`, `isatty`, the clock
(`time`, `gettimeofday`, `clock_gettime`, `times`, `nanosleep`) and `_exit`
use INT 21h. DOS error codes become `errno` values in `error.c`.

- Handles are in text or binary mode, as with DOS C compilers. In text
  mode, writes turn LF into CR LF, and reads turn CR LF into LF and stop
  at Ctrl-Z (a CR with no LF after it is kept). Binary mode reads and
  writes bytes as they are.
  - `fopen` opens a file in binary mode with `b` in the mode (`"rb"`,
    `"w+b"`), and in text mode otherwise (`t`, as in `"wt"`, also asks
    for text).
  - `open` takes `O_TEXT` or `O_BINARY`; with neither, it uses `_fmode`,
    which starts as `O_TEXT`.
  - `setmode(fd, O_TEXT or O_BINARY)` (also `_setmode`, declared in
    `<io.h>` and `<fcntl.h>`) changes a handle's mode and returns the old
    one.
  - The standard handles 0 to 4 start in text mode.
  - `ftell` and `fseek` on a text stream use file offsets, so `ftell`
    counts each line end as 2 bytes. It assumes each LF still in the
    stdio buffer came from a CR LF.
  - After a read stops at Ctrl-Z, the file stays at the Ctrl-Z and
    further reads return 0 until the program seeks.
- Creating a file with `O_TRUNC` (`fopen(..., "w")`) also writes 0 bytes
  at its start, which truncates it on emulators (emu2) that don't empty
  an existing file on INT 21h function 3Ch.
- `O_APPEND` seeks to the end before each write.
- The DOS clock is local time; it is treated as UTC. It ticks about 18
  times a second.
- `fork`, `execve`, `pipe`, `waitpid` and `getentropy` fail with
  `ENOSYS` (`waitpid` with `ECHILD`); terminal attributes with `ENOTTY`.
  Without `getentropy`, `arc4random` runs from a fixed all-zero seed, so
  its output is predictable. A program can define `arc4random_abort` to
  be called instead.
- Signals, `raise` and `sysconf` come from `libos/fallback`.
- `halloc(n, size)` and `hfree` (`halloc.c`, declared in `<malloc.h>`
  for every model but tiny) allocate zeroed blocks of up to 1 MiB with
  INT 21h function 48h, last fit, and free them with function 49h. DOS
  gives an .EXE all of its memory, so when no free memory is left the
  first `halloc` shrinks the program's block with function 4Ah: to the end
  of the heap, or in the huge models by the size of the block, lowering
  the end of the heap.
- `dosmem.c` gets memory from DOS for `halloc` and the far heap; the
  tiny model gives up all but its 64 KiB segment. `farstring.c` has the
  far string functions (`_fmemcpy`, `_fstrlen` and the rest), `farheap.c`
  the far heap (`_fmalloc`: arenas of `arena.h` in segments from DOS or
  `GlobalAlloc` with near data, `malloc` with far data), and
  `nearheap.c` the near heap (`_nmalloc`: `malloc` with near data, the
  Windows local heap, or an arena in `__msdos_near_heap` in DGROUP,
  `nearheapsize.c`).

## Interrupt handlers and resident programs

`<dos.h>` and `<i86.h>` (in `libc/machine/i8086`) have what Open Watcom
and Microsoft C have for interrupt handlers and TSRs, for DOS programs:

- `_dos_getvect(n)` and `_dos_setvect(n, handler)` (`dosvect.c`) read and
  set interrupt vector `n` with INT 21h functions 35h and 25h. Handlers
  are `void (__interrupt __far *)()`.
- `_dos_keep(status, paras)` (`dosvect.c`) ends the program and keeps
  `paras` paragraphs of it, from the PSP, resident (INT 21h function 31h).
  With 0 it keeps the program up to the end of its heap (the break of
  `sbrk`), which covers the code, the data and anything allocated so far;
  the stack of a .COM, at the top of its segment, is given back, since
  interrupt handlers run on the stack of the program they interrupt. Other
  DOS blocks (`halloc`, the far heap) stay allocated. `atexit` functions
  and destructors do not run.
- `_chain_intr(handler)` (`chainintr.S`), from an `__interrupt` function,
  restores the registers it saved, with any changes made to them, and
  jumps to `handler` as if the interrupt had gone there. It finds the
  saved registers at the interrupt function's BP.
- `union INTPACK` (`<i86.h>`) is the layout of the saved registers that an
  `__interrupt` function can take as its parameter. `_disable()` and
  `_enable()` are CLI and STI; `MK_FP`, `FP_SEG` and `FP_OFF` make and
  split far pointers. `_psp` is the PSP segment.

## 16-bit Windows

The same code builds for `i8086-pc-win16` (the cross files
`scripts/cross-clang-i8086-win16-<model>.txt`, for small, medium, compact
and large), where `__WINDOWS__` is defined. Windows programs call DOS for
files and the clock, so the system calls above work unchanged; the
startup code and the heap differ:

- `crt0.S` calls `InitTask`, which sets up the program (DS is DGROUP and
  SS is DS), clears BSS, saves the instance handles and `nCmdShow`, calls
  `WaitEvent` and `InitApp` and goes on to `__msdos_start` as on DOS. The
  first 16 bytes of DGROUP are left free for Windows. There is no stack of
  the program's own; the linker's `option stack` sets its size.
- The heap (`win16.c`) is one `GlobalAlloc` block of up to 64 KiB in the
  compact and large models, or a `LocalAlloc` block in DGROUP in small and
  medium.
- `argv[0]` comes from `GetModuleFileName`. The command line is the PSP
  tail, as on DOS.
- `winmain.c` has a `main` that calls `WinMain(hInstance, hPrevInstance,
  lpCmdLine, nCmdShow)`, for programs without a `main` of their own.
- `libentry.S` is a DLL's startup code (`libentry.obj`): it sets up the
  local heap with `LocalInit`, clears BSS and calls `__win16_libstart`,
  which runs the constructors and calls `LibMain` (a default one returning
  1 is in `libmain.c`). `WEP` (`wep.c`) runs the destructors.
- `_exit` is INT 21h function 4Ch, which Windows handles for programs.
