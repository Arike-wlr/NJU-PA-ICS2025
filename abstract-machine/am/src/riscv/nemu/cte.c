#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;
void __am_get_cur_as(Context *c);
void __am_switch(Context *c);



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

Context* __am_irq_handle(Context *c) {
  __am_get_cur_as(c);
  if (user_handler) { // 检查是否有注册的事件处理函数
    Event ev = {0};   // 初始化事件结构体
    printf("mcause = %d", c->mcause);
    switch (c->mcause) {
      case -1: {ev.event = EVENT_YIELD;c->mepc += 4; break;}
      case 0:
      case 1:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:case 10:
      case 11:case 12:case 13:case 14:case 15:case 16:case 17:case 18:case 19:
      {ev.event = EVENT_SYSCALL;c->mepc += 4; break;}
      default: {ev.event = EVENT_ERROR; break;}
    }
    c = user_handler(ev, c); // 调用用户注册的处理函数
    assert(c != NULL); // 确保返回有效的Context
  }
  __am_switch(c);
  return c; // 返回Context指针
}