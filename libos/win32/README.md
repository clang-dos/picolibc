# picolibc on 32-bit Windows

This directory lets picolibc stand in for msvcrt.dll or crtdll.dll as the
C library of 32-bit Windows programs. It goes into `libc.a`, and a program
linked with it imports only `kernel32.dll`, so it runs on Windows 95, 98
and ME, and Windows NT 3.1 and later, with
nothing to install. It is built for the `i386-pc-windows*-gnu` targets of
the [clang-dos LLVM toolchain](https://github.com/clang-dos/llvm), and
libc++ builds on it with `RUNTIMES_USE_LIBC=picolibc`.

## Building

With clang, `llvm-ar` and the other LLVM tools on `PATH`:

```sh
meson setup build-win32 \
    --cross-file scripts/cross-clang-i386-windows.txt \
    -Dos-win32=true -Denable-malloc=false -Dinitfini-array=false \
    -Dmultilib=false -Dpicocrt=false -Dsemihost=false -Dtests=false \
    -Dspecsdir=none -Dposix-console=true -Dmb-capable=true \
    -Dio-wchar=true -Dio-pos-args=true -Dio-long-double=true \
    --prefix=$PWD/install-win32
ninja -C build-win32 install
```

The headers go over the mingw-w64 API headers (windows.h and the rest)
with mingw-w64's C library headers taken out; the win32-sdk repository's
build script puts such a sysroot together. Programs link with
`-nostartfiles -nodefaultlibs` and then `-lc -lclang_rt.builtins-i386
-lkernel32`, and are compiled with `-fno-emulated-tls`.

## What it does

- `start.c` has the entry points the linker looks for, `mainCRTStartup`
  and `WinMainCRTStartup`. It splits the command line with the Microsoft C
  rules, copies the environment, runs the constructors (`ctors.c`, from
  clang's `__main` call) and calls `main` or `WinMain`.
- `tlssup.c` is the TLS directory, so `__thread` variables (errno among
  them) are native TLS, set up by Windows for every thread.
  `cxa_thread_atexit.c` runs C++ `thread_local` destructors when a thread
  ends, and `keydtor.c` runs destructors for `TlsAlloc` keys, which libc++
  uses on Windows before Server 2003.
- Descriptors (`fd.c`) are a table of Win32 handles. Files open in text
  mode unless `O_BINARY`, `_fmode` or `setmode` say otherwise: `read`
  turns CR LF into LF and stops at Ctrl-Z, and `write` turns LF into
  CR LF. Consoles, pipes and files all go through `ReadFile` and
  `WriteFile`. `_wopen` and `_wfopen` open files by wide name, with
  `CreateFileW` on Windows NT and the ANSI name on Windows 9x.
- `malloc` uses the process heap (`HeapAlloc`); `lock.c` makes picolibc's
  locks critical sections, so the library is thread safe.
- Time comes from `GetSystemTime` (there is no `GetSystemTimeAsFileTime`
  on Windows NT 3.1), and the monotonic clock from
  `QueryPerformanceCounter`, or `GetTickCount` where there is none.
- `_beginthreadex` and the rest of `<process.h>` start threads with
  `CreateThread`. `<io.h>` has `_get_osfhandle` and `_open_osfhandle`.
- Error codes from `GetLastError` become errno values in `error.c`.

`link` and `symlink` fail with `ENOSYS`, and there are no signals beyond
`raise`.
