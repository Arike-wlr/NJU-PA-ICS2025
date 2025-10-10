#include <am.h>
#include <nemu.h>

extern char _heap_start;
int main(const char *args);

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
//结构用于指示堆区的起始和末尾地址

void putch(char ch) { //用于输出一个字符
  outb(SERIAL_PORT, ch);
}

void halt(int code) { //用于结束程序的运行
  nemu_trap(code);

  // should not reach here
  while (1);
}

void _trm_init() { //用于进行TRM相关的初始化工作
  int ret = main(mainargs);
  halt(ret);
}
