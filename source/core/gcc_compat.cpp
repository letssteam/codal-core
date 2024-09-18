#include "gcc_compat.h"

#include "CodalAssert.h"
#include "codal_target_hal.h"

#if __GNUC__ > 11
extern "C" {
__attribute__((weak)) int _close(int fd)
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}

__attribute__((weak)) int _getpid()
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}

__attribute__((weak)) int _kill(int pid, int sig)
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}

__attribute__((weak)) int _lseek(int file, int ptr, int dir)
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}

__attribute__((weak)) int _read(int file, char* ptr, int len)
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}
__attribute__((weak)) int _write(int file, char* ptr, int len)
{
    assert_fault("Newlib syscalls are not supported!");
    return -1;
}
}
#endif