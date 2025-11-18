#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;

Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    printf("mcause = %d, mepc = 0x%x\n", c->mcause, c->mepc);
    switch (c->mcause) {
      
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  /*CTE（ConText Extension,上下文扩展）的初始化函数,AM中异常处理机制的初始化入口*/

  // initialize exception entry,将异常入口地址设置为__am_asm_trap
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler,注册用户自定义的异常处理函数,保存操作系统提供的事件处理函数指针
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
