#include <common.h>
#include "syscall.h"
#include <sys/time.h>
#include <proc.h>

size_t fs_read(int fd, void* buf, size_t len);
size_t fs_write(int fd,const void* buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
size_t fs_close(int fd);
size_t fs_open(const char *pathname, int flags, int mode);
int mm_brk(uintptr_t brk);
void naive_uload(PCB *pcb, const char *filename);
static char curr_pathname[64] = IMAGE_FILE;
size_t context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
void switch_boot_pcb(void);

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
  //Log("Syscall ID = %d, gpr2 = %d, gpr3 = %d, gpr4 = %d", c->GPR1,a[1],a[2],a[3]);
  switch (a[0]) {
    case SYS_yield:{
    Log("SYS_yield called");
      yield();
      c->GPRx = 0;
      break;
    }

    case SYS_exit:{
      Log("SYS_exit called with code %d", a[1]);
      if(strcmp("/bin/menu", IMAGE_FILE) == 0) naive_uload(NULL, "/bin/menu");
      else if(strcmp("/bin/nterm", IMAGE_FILE) == 0 && strcmp("/bin/nterm", curr_pathname) != 0) {
        strncpy(curr_pathname, "/bin/nterm", 11);
        naive_uload(NULL, "/bin/nterm");
      }
      
      halt(a[1]);
      c->GPRx = 0;
      break;
    }

    case SYS_open:{
      Log("SYS_open called with pathname=%s, flags=%d, mode=%d",(char*)a[1], a[2], a[3]);
      c->GPRx = fs_open((char*)a[1], a[2], a[3]);
      break;
    }

    case SYS_read:{
      Log("SYS_read called with fd=%d, buf=%x, len=%d", (int)a[1], (void *)a[2], (size_t)a[3]);
      c->GPRx= fs_read(a[1],(void*) a[2], a[3]);
      break;
    }

    case SYS_write:{
      Log("SYS_write called with fd=%d, buf=%x, len=%d", (int)a[1], (void *)a[2], (size_t)a[3]);
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
      Log("SYS_brk called with addr=%x,increment=%x\n", (void *)a[1]);
      for (uint32_t i=0; i<(int32_t)a[2]; i++) {
        *(uint32_t*)(a[1] + i) = 0;
      }
      c->GPRx = 0;
      break;
    }
    
    case SYS_execve:{
      Log("SYS_execve called with filename=%p, argv=%p, envp=%p", (void *)a[1], (void *)a[2], (void *)a[3]);
      strncpy(curr_pathname, (char*)(a[1]), 1+strlen((char*)(a[1])));
      // naive_uload(NULL, (char*)(a[1]));
      size_t ret = context_uload(NULL, (char*)(a[1]), (char* const*)a[2], (char* const*)a[3]);
      if (ret == -2){c->GPRx = -2; break;}
      c->GPRx = 0;
      switch_boot_pcb();
      yield();
      break;
    }

    case SYS_gettimeofday:{
      Log("SYS_gettimeofday called with tv=%p, tz=%p", (void *)a[1], (void *)a[2]);
      uint32_t tick = io_read(AM_TIMER_UPTIME).us;
      ((struct timeval *)a[1])->tv_usec = tick;
      ((struct timeval *)a[1])->tv_sec = tick / 1000;
      c->GPRx = 0;
      break;
    }
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
  //Log("SYS_call returning %d", c->GPRx);
}
