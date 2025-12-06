#include <common.h>

//IRQ 的全称是 Interrupt ReQuest（中断请求）
void do_syscall(Context *c);

static Context* do_event(Event e, Context* c) {
  /*事件处理函数*/
  //Log("Handling event ID = %d", e.event);
  switch (e.event) {
    case EVENT_YIELD:
      Log("EVENT_YIELD received");
      //c = schedule(c);
      break;
    case EVENT_SYSCALL:
      Log("EVENT_SYSCALL received");
      do_syscall(c);
      break;
    case EVENT_IRQ_TIMER: 
    Log("EVENT_IRQ_TIMER received");
      //c = schedule(c);
      break;
    default: panic("Unhandled event ID = %d", e.event);
  }
  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
  //调用cte_init()注册do_event为事件处理回调,完成CTE的初始化
}
