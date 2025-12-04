#include <common.h>
#include "syscall.h"

size_t fs_write(int fd, const void *buf, size_t len);

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  Log("Syscall ID = %d", a[0]);
  switch (a[0]) {
    case SYS_yield:
      Log("SYS_yield called");
      yield();
      c->GPRx = 0;
      break;
    case SYS_exit:
      Log("SYS_exit called with code %d", a[1]);
      halt(a[1]);
      c->GPRx = 0;
      break;
    case SYS_write:
      Log("SYS_write called with fd=%d, buf=%p, len=%d", (int)a[1], (void *)a[2], (size_t)a[3]);
      c->GPRx = fs_write((int)a[1], (void *)a[2], (size_t)a[3]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
