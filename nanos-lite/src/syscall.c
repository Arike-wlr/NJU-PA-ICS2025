#include <common.h>
#include "syscall.h"

void do_syscall(Context *c) {
  
  uintptr_t a[4];
  Log("Here? in do_syscall");
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  Log("Syscall ID = %d", a[0]);
  switch (a[0]) {
    case SYS_yield:
      yield();
      c->GPRx = 0;
      break;
    case SYS_exit:
      halt(a[1]);
      c->GPRx = 0;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
