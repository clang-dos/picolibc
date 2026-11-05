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

#ifndef _LOCAL_WIN32_H_
#define _LOCAL_WIN32_H_

#define _GNU_SOURCE

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * The parts of the Win32 API that the C library uses, written out here so
 * that it builds without the Windows headers. Everything comes from
 * kernel32.dll, and only from functions that Windows 95 and NT 3.1 have,
 * unless a comment says the function is looked up at run time.
 */

#define __WIN32_API __attribute__((__dllimport__, __stdcall__))

typedef void         *__win32_handle;
typedef unsigned long __win32_dword;
typedef int           __win32_bool;

#define __WIN32_INVALID_HANDLE ((__win32_handle)(intptr_t)-1)

struct __win32_filetime {
    __win32_dword low, high;
};

struct __win32_systemtime {
    uint16_t year, month, day_of_week, day, hour, minute, second, milliseconds;
};

struct __win32_find_data {
    __win32_dword           attributes;
    struct __win32_filetime creation, access, write;
    __win32_dword           size_high, size_low;
    __win32_dword           reserved0, reserved1;
    char                    name[260];
    char                    short_name[14];
};

struct __win32_file_info {
    __win32_dword           attributes;
    struct __win32_filetime creation, access, write;
    __win32_dword           volume_serial;
    __win32_dword           size_high, size_low;
    __win32_dword           links;
    __win32_dword           index_high, index_low;
};

struct __win32_startup_info {
    __win32_dword  cb;
    char          *reserved, *desktop, *title;
    __win32_dword  x, y, x_size, y_size, x_count_chars, y_count_chars;
    __win32_dword  fill_attribute, flags;
    uint16_t       show_window, reserved2;
    unsigned char *reserved3;
    __win32_handle std_input, std_output, std_error;
};

struct __win32_process_info {
    __win32_handle process, thread;
    __win32_dword  process_id, thread_id;
};

struct __win32_security_attributes {
    __win32_dword length;
    void         *descriptor;
    __win32_bool  inherit;
};

/* CRITICAL_SECTION: 24 bytes on 32-bit Windows, 40 on 64-bit. */
struct __win32_critical_section {
    void          *debug_info;
    long           lock_count, recursion_count;
    __win32_handle owning_thread, lock_semaphore;
    uintptr_t      spin_count;
};

#define __WIN32_STD_INPUT_HANDLE           ((__win32_dword) - 10)
#define __WIN32_STD_OUTPUT_HANDLE          ((__win32_dword) - 11)
#define __WIN32_STD_ERROR_HANDLE           ((__win32_dword) - 12)

#define __WIN32_GENERIC_READ               0x80000000UL
#define __WIN32_GENERIC_WRITE              0x40000000UL
#define __WIN32_FILE_SHARE_READ            0x00000001UL
#define __WIN32_FILE_SHARE_WRITE           0x00000002UL
#define __WIN32_FILE_SHARE_DELETE          0x00000004UL
#define __WIN32_CREATE_NEW                 1
#define __WIN32_CREATE_ALWAYS              2
#define __WIN32_OPEN_EXISTING              3
#define __WIN32_OPEN_ALWAYS                4
#define __WIN32_TRUNCATE_EXISTING          5
#define __WIN32_FILE_ATTRIBUTE_READONLY    0x00000001UL
#define __WIN32_FILE_ATTRIBUTE_HIDDEN      0x00000002UL
#define __WIN32_FILE_ATTRIBUTE_SYSTEM      0x00000004UL
#define __WIN32_FILE_ATTRIBUTE_DIR         0x00000010UL
#define __WIN32_FILE_ATTRIBUTE_NORMAL      0x00000080UL
#define __WIN32_FILE_ATTRIBUTE_REPARSE     0x00000400UL
#define __WIN32_INVALID_FILE_ATTRIBUTES    ((__win32_dword) - 1)
#define __WIN32_FILE_FLAG_BACKUP_SEMANTICS 0x02000000UL

#define __WIN32_FILE_BEGIN                 0
#define __WIN32_FILE_CURRENT               1
#define __WIN32_FILE_END                   2
#define __WIN32_INVALID_SET_FILE_POINTER   ((__win32_dword) - 1)

#define __WIN32_FILE_TYPE_UNKNOWN          0
#define __WIN32_FILE_TYPE_DISK             1
#define __WIN32_FILE_TYPE_CHAR             2
#define __WIN32_FILE_TYPE_PIPE             3

#define __WIN32_HEAP_ZERO_MEMORY           0x00000008UL

#define __WIN32_DUPLICATE_SAME_ACCESS      0x00000002UL
#define __WIN32_STARTF_USESHOWWINDOW       0x00000001UL
#define __WIN32_STARTF_USESTDHANDLES       0x00000100UL
#define __WIN32_SW_SHOWDEFAULT             10
#define __WIN32_INFINITE                   0xffffffffUL
#define __WIN32_HANDLE_FLAG_INHERIT        0x00000001UL

#define __WIN32_ERROR_FILE_NOT_FOUND       2
#define __WIN32_ERROR_NO_MORE_FILES        18
#define __WIN32_ERROR_HANDLE_EOF           38
#define __WIN32_ERROR_BROKEN_PIPE          109
#define __WIN32_ERROR_CALL_NOT_IMPLEMENTED 120
#define __WIN32_CP_ACP                     0
#define __WIN32_ERROR_ALREADY_EXISTS       183

/* For stdio: whether a descriptor translates line ends. */
int                        __win32_is_text(int fd);

__WIN32_API __win32_handle GetStdHandle(__win32_dword which);
__WIN32_API __win32_bool   SetStdHandle(__win32_dword which, __win32_handle h);
__WIN32_API __win32_handle CreateFileA(const char *name, __win32_dword access, __win32_dword share,
                                       void *security, __win32_dword disposition,
                                       __win32_dword flags, __win32_handle template_file);
__WIN32_API __win32_dword  GetVersion(void);
__WIN32_API int WideCharToMultiByte(unsigned int codepage, __win32_dword flags, const wchar_t *wide,
                                    int nwide, char *multi, int nmulti, const char *defchar,
                                    __win32_bool *useddef);
__WIN32_API __win32_handle CreateThread(void *security, __win32_dword                  stack,
                                        __win32_dword(__stdcall *start)(void *), void *arg,
                                        __win32_dword flags, __win32_dword *id);
__WIN32_API void           ExitThread(__win32_dword status) __attribute__((__noreturn__));
__WIN32_API __win32_bool ReadFile(__win32_handle h, void *buf, __win32_dword n, __win32_dword *done,
                                  void *overlapped);
__WIN32_API __win32_bool WriteFile(__win32_handle h, const void *buf, __win32_dword n,
                                   __win32_dword *done, void *overlapped);
__WIN32_API __win32_bool CloseHandle(__win32_handle h);
__WIN32_API __win32_dword  SetFilePointer(__win32_handle h, long low, long *high,
                                          __win32_dword method);
__WIN32_API __win32_dword  GetFileType(__win32_handle h);
__WIN32_API __win32_bool   GetFileInformationByHandle(__win32_handle            h,
                                                      struct __win32_file_info *info);
__WIN32_API __win32_bool   SetEndOfFile(__win32_handle h);
__WIN32_API __win32_bool   FlushFileBuffers(__win32_handle h);
__WIN32_API __win32_bool   SetFileTime(__win32_handle h, const struct __win32_filetime *creation,
                                       const struct __win32_filetime *access,
                                       const struct __win32_filetime *write);
__WIN32_API __win32_bool   DuplicateHandle(__win32_handle from_process, __win32_handle h,
                                           __win32_handle to_process, __win32_handle *out,
                                           __win32_dword access, __win32_bool inherit,
                                           __win32_dword options);
__WIN32_API __win32_bool   CreatePipe(__win32_handle *read, __win32_handle *write,
                                      struct __win32_security_attributes *security,
                                      __win32_dword                       size);
__WIN32_API __win32_handle GetCurrentProcess(void);
__WIN32_API __win32_dword  GetCurrentProcessId(void);
__WIN32_API __win32_dword  GetCurrentThreadId(void);
__WIN32_API __win32_dword  GetLastError(void);
__WIN32_API void           SetLastError(__win32_dword error);
__WIN32_API void           ExitProcess(unsigned int status) __attribute__((__noreturn__));
__WIN32_API char          *GetCommandLineA(void);
__WIN32_API char          *GetEnvironmentStrings(void);
__WIN32_API __win32_bool   SetEnvironmentVariableA(const char *name, const char *value);
__WIN32_API __win32_dword GetModuleFileNameA(__win32_handle module, char *name, __win32_dword size);
__WIN32_API __win32_handle GetModuleHandleA(const char *name);
__WIN32_API void           GetStartupInfoA(struct __win32_startup_info *info);
__WIN32_API __win32_handle LoadLibraryA(const char *name);
__WIN32_API void          *GetProcAddress(__win32_handle module, const char *name);

__WIN32_API __win32_handle GetProcessHeap(void);
__WIN32_API void          *HeapAlloc(__win32_handle heap, __win32_dword flags, size_t n);
__WIN32_API void          *HeapReAlloc(__win32_handle heap, __win32_dword flags, void *p, size_t n);
__WIN32_API __win32_bool   HeapFree(__win32_handle heap, __win32_dword flags, void *p);
__WIN32_API size_t         HeapSize(__win32_handle heap, __win32_dword flags, const void *p);

__WIN32_API __win32_dword  GetFileAttributesA(const char *name);
__WIN32_API __win32_bool   SetFileAttributesA(const char *name, __win32_dword attributes);
__WIN32_API __win32_handle FindFirstFileA(const char *pattern, struct __win32_find_data *data);
__WIN32_API __win32_bool   FindNextFileA(__win32_handle h, struct __win32_find_data *data);
__WIN32_API __win32_bool   FindClose(__win32_handle h);
__WIN32_API __win32_bool   DeleteFileA(const char *name);
__WIN32_API __win32_bool   MoveFileA(const char *from, const char *to);
__WIN32_API __win32_bool   CreateDirectoryA(const char *name, void *security);
__WIN32_API __win32_bool   RemoveDirectoryA(const char *name);
__WIN32_API __win32_bool   SetCurrentDirectoryA(const char *name);
__WIN32_API __win32_dword  GetCurrentDirectoryA(__win32_dword size, char *buf);
__WIN32_API __win32_dword  GetFullPathNameA(const char *name, __win32_dword size, char *buf,
                                            char **file_part);
__WIN32_API __win32_bool   GetDiskFreeSpaceA(const char *root, __win32_dword *sectors_per_cluster,
                                             __win32_dword *bytes_per_sector,
                                             __win32_dword *free_clusters,
                                             __win32_dword *total_clusters);

__WIN32_API void           GetSystemTime(struct __win32_systemtime *t);
__WIN32_API __win32_bool   SystemTimeToFileTime(const struct __win32_systemtime *t,
                                                struct __win32_filetime         *ft);
__WIN32_API __win32_dword  GetTickCount(void);
__WIN32_API __win32_bool   QueryPerformanceCounter(int64_t *count);
__WIN32_API __win32_bool   QueryPerformanceFrequency(int64_t *frequency);
__WIN32_API void           Sleep(__win32_dword ms);

__WIN32_API void          *TlsGetValue(__win32_dword key);
__WIN32_API void           InitializeCriticalSection(struct __win32_critical_section *cs);
__WIN32_API void           EnterCriticalSection(struct __win32_critical_section *cs);
__WIN32_API void           LeaveCriticalSection(struct __win32_critical_section *cs);
__WIN32_API void           DeleteCriticalSection(struct __win32_critical_section *cs);

__WIN32_API __win32_bool   CreateProcessA(const char *application, char *command_line,
                                          struct __win32_security_attributes *process_security,
                                          struct __win32_security_attributes *thread_security,
                                          __win32_bool inherit, __win32_dword flags, void *env,
                                          const char *directory, struct __win32_startup_info *startup,
                                          struct __win32_process_info *info);
__WIN32_API __win32_dword  WaitForSingleObject(__win32_handle h, __win32_dword ms);
__WIN32_API __win32_bool   GetExitCodeProcess(__win32_handle h, __win32_dword *status);

/* Set errno from a Win32 error code (GetLastError() if code is 0) and
 * return -1. */
int                        __win32_error(__win32_dword code);

/* Time: FILETIME counts 100 ns since 1601. */
#define __WIN32_EPOCH_DIFF 116444736000000000LL /* 1601 to 1970 in 100 ns */
int64_t __win32_filetime(const struct __win32_filetime *ft);
void    __win32_to_filetime(int64_t t, struct __win32_filetime *ft);
/* The time now, as a FILETIME count. */
int64_t __win32_now(void);

/*
 * File descriptors. Each one holds a Win32 handle and the state that
 * Windows does not keep: text mode and append.
 */
#define __WIN32_MAX_FD     256
#define __WIN32_FD_OPEN    0x01
#define __WIN32_FD_APPEND  0x02 /* O_APPEND: seek to the end before writes. */
#define __WIN32_FD_TEXT    0x04 /* O_TEXT: CR LF line ends, Ctrl-Z ends. */
#define __WIN32_FD_EOF     0x08 /* A text read reached Ctrl-Z. */
#define __WIN32_FD_DEVICE  0x10 /* A console or other character device. */
#define __WIN32_FD_PIPE    0x20 /* A pipe. */
#define __WIN32_FD_NOCLOSE 0x40 /* A standard handle, which close() keeps open. */
#define __WIN32_FD_PENDING 0x80 /* pending holds a byte read ahead. */

struct __win32_fd {
    __win32_handle handle;
    unsigned char  flags;
    /* A text read from a pipe or device that ended with CR reads one byte
     * more to look for LF. When that byte is not LF it is kept here for
     * the next read, with __WIN32_FD_PENDING set. */
    char           pending;
};

extern struct __win32_fd __win32_fds[__WIN32_MAX_FD];

/* The descriptor's handle, or NULL with errno set to EBADF. */
__win32_handle           __win32_handle_of(int fd);
/* The lowest free descriptor for a handle, or -1 with errno EMFILE. */
int                      __win32_new_fd(__win32_handle h, int flags);
/* Flags for a handle from GetFileType. */
int                      __win32_type_flags(__win32_handle h);
void                     __win32_init_fds(void);

static inline int
__win32_fdflags(int fd)
{
    return fd >= 0 && fd < __WIN32_MAX_FD ? __win32_fds[fd].flags : 0;
}

/* Process startup, from start.c. */
extern __win32_handle __win32_heap;
void                  __win32_init(void);
void                  __win32_init_args(int *argc, char ***argv);

#endif /* _LOCAL_WIN32_H_ */
