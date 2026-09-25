/*
 * ipod_syscalls.c - the OS layer newlib-nano calls into.
 *
 * Memory is handled in z_ipod.c; files come from the synchronous DebugUtil calls.
 */
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stddef.h>
#include <sys/stat.h>
#include "eapp.h"

#undef errno
extern int errno;

// Set by doomgeneric_ipod.c around every call into Doom; exit() lands there.
extern jmp_buf ipod_exit_jmp;

// malloc and friends live in z_ipod.c (on Doom's zone), so newlib never grows a heap.
void* _sbrk(ptrdiff_t incr) {
    (void)incr;
    errno = ENOMEM;
    return (void*)-1;
}

void _exit(int status) {
    longjmp(ipod_exit_jmp, status ? status : 1);
}

int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }

// newlib is built with unwind tables that name these; nothing here throws, so
// empty ones keep libgcc's whole unwinder out of the image.
void __aeabi_unwind_cpp_pr0(void) {}
void __aeabi_unwind_cpp_pr1(void) {}
void __aeabi_unwind_cpp_pr2(void) {}

// Read-only access to the game's own folder (where doom1.wad is installed) through the
// synchronous DebugUtil file calls. Writes (config, saves) are refused for now.
#define FIRST_FD 3
#define MAX_FILES 10   // the OS allows 10 open files per game

static eapp_file* files[MAX_FILES];

static eapp_file* fd_file(int fd) {
    if (fd < FIRST_FD || fd >= FIRST_FD + MAX_FILES)
        return NULL;
    return files[fd - FIRST_FD];
}

int _open(const char* path, int flags, int mode) {
    (void)mode;
    if (flags & (O_WRONLY | O_RDWR | O_CREAT | O_TRUNC | O_APPEND)) {
        errno = EROFS;
        return -1;
    }
    int slot = 0;
    while (slot < MAX_FILES && files[slot])
        slot++;
    if (slot == MAX_FILES) {
        errno = EMFILE;
        return -1;
    }
    eapp_file* f = NULL;
    if (eapp_file_open(EAPP_LOC_GAME, path, EAPP_FILE_READ, &f) != 0 || !f) {
        errno = ENOENT;
        return -1;
    }
    files[slot] = f;
    return FIRST_FD + slot;
}

int _close(int fd) {
    eapp_file* f = fd_file(fd);
    if (!f)
        return -1;
    eapp_file_close(f);
    files[fd - FIRST_FD] = NULL;
    return 0;
}

int _read(int fd, void* buf, size_t n) {
    eapp_file* f = fd_file(fd);
    if (!f) {
        errno = EBADF;
        return -1;
    }
    uint32_t done = 0;
    int err = eapp_file_read(f, buf, n, &done);
    if (err == 0 || err == 5)   // 5: nothing left to read
        return (int)done;
    errno = EIO;
    return -1;
}

int _lseek(int fd, int off, int whence) {
    eapp_file* f = fd_file(fd);
    if (!f) {
        errno = EBADF;
        return -1;
    }
    if (eapp_file_seek(f, off, whence) != 0) {
        errno = EINVAL;
        return -1;
    }
    return (int)eapp_file_tell(f);
}

int _fstat(int fd, struct stat* st) {
    eapp_file* f = fd_file(fd);
    if (f) {
        st->st_mode = S_IFREG;
        st->st_size = eapp_file_size(f);
    } else {
        st->st_mode = S_IFCHR;
    }
    return 0;
}

int _isatty(int fd) { return fd <= 2; }
int _unlink(const char* path) { (void)path; errno = ENOENT; return -1; }
int _rename(const char* a, const char* b) { (void)a; (void)b; errno = ENOENT; return -1; }
int _link(const char* a, const char* b) { (void)a; (void)b; errno = EROFS; return -1; }
int mkdir(const char* path, mode_t mode) { (void)path; (void)mode; errno = EROFS; return -1; }

// stdout/stderr: no console, so keep the tail of the output for a debugger to find.
char ipod_log[4096];
static unsigned ipod_log_pos;

int _write(int fd, const void* buf, size_t n) {
    (void)fd;
    const char* s = buf;
    for (size_t i = 0; i < n; i++)
        ipod_log[ipod_log_pos++ % sizeof(ipod_log)] = s[i];
    return (int)n;
}

// Best effort: save the log to iPod_Control/gamedata_RW/<GUID>/doom.log.
void ipod_log_save(void) {
    eapp_file* f = NULL;
    if (eapp_file_open(EAPP_LOC_DATA, "doom.log", EAPP_FILE_WRITE, &f) != 0 || !f)
        return;
    uint32_t done;
    unsigned n = sizeof(ipod_log), start = ipod_log_pos % n;
    if (ipod_log_pos >= n)
        eapp_file_write(f, ipod_log + start, n - start, &done);
    eapp_file_write(f, ipod_log, start, &done);
    eapp_file_close(f);
}
