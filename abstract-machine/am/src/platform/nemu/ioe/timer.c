#include <am.h>
#include <nemu.h>

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 读取 RTC 地址处的低 32 位和高 32 位时间戳（两个寄存器一共 64 位）
  volatile uint32_t low_rtc = inl(RTC_ADDR + 0x00);
  volatile uint32_t high_rtc = inl(RTC_ADDR + 0x04);
  uptime->us = (uint64_t)high_rtc << 32 | low_rtc;
  //uptime->us = 0;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 38;
  rtc->hour   = 19;
  rtc->day    = 25;
  rtc->month  = 12;
  rtc->year   = 1991;
}
