#include <common.h>
#include "syscall.h"

size_t fs_read(int fd, void* buf, size_t len);
size_t fs_write(int fd,const void* buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
size_t fs_close(int fd);
size_t fs_open(const char *pathname, int flags, int mode);
int mm_brk(uintptr_t brk);

size_t sys_write(int fd, const void* buf, size_t len) {
  if (fd == 1 || fd == 2) {
    for (size_t i = 0; i < len; i++) {
      putch(((char*)buf)[i]);
    }
    return len;
  }
  return fs_write(fd, buf, len);
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;
  Log("Syscall ID = %d, gpr2 = %d, gpr3 = %d, gpr4 = %d", a[0],a[1],a[2],a[3]);
  switch (a[0]) {
    case SYS_yield:{
    Log("SYS_yield called");
      yield();
      c->GPRx = 0;
      break;
    }

    case SYS_exit:{
      Log("SYS_exit called with code %d", a[1]);
      halt(a[1]);
      c->GPRx = 0;
      break;
    }

    case SYS_open:{
      Log("SYS_open called with SYScall ID= %d, pathname=%p, flags=%d, mode=%d",c->GPR1, (char*)a[1], a[2], a[3]);
      c->GPRx = fs_open((char*)a[1], a[2], a[3]);
      break;
    }

    case SYS_read:{
      Log("SYS_read called with fd=%d, buf=%p, len=%d", (int)a[1], (void *)a[2], (size_t)a[3]);
      c->GPRx= fs_read(a[1],(void*) a[2], a[3]);
      break;
    }

    case SYS_write:{
      Log("SYS_write called with fd=%d, buf=%p, len=%d", (int)a[1], (void *)a[2], (size_t)a[3]);
      c->GPRx = sys_write((int)a[1], (void *)a[2], (size_t)a[3]);
      break;
    }  
    
    case SYS_close:{
      Log("SYS_close called with fd=%d", (int)a[1]);
      c->GPRx = fs_close(a[1]);
      break;
    }
    
    case SYS_lseek:{
      Log("SYS_lseek called with fd=%d, offset=%d, whence=%d", (int)a[1], (size_t)a[2], (int)a[3]);
      c->GPRx = fs_lseek(a[1], a[2], a[3]);
      break;
    }

    case SYS_brk:{
      Log("SYS_brk called with addr=%p", (void *)a[1]);
      c->GPRx = mm_brk((uintptr_t)a[1]);
      break;
    }
    
    case SYS_execve:{
      Log("SYS_execve called with filename=%p, argv=%p, envp=%p", (void *)a[1], (void *)a[2], (void *)a[3]);
      panic("Not implemented");
      break;
    }

    case SYS_gettimeofday:{
      Log("SYS_gettimeofday called with tv=%p, tz=%p", (void *)a[1], (void *)a[2]);
      panic("Not implemented");
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  Log("SYS_call returning %d", c->GPRx);
}
