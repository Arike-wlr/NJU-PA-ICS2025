#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000 // 从键盘扫描码或按键状态值中提取按键按下/释放的信息

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  int k = inb(KBD_ADDR);
  kbd->keydown = (k & KEYDOWN_MASK ? true : false);
  kbd->keycode = k & ~KEYDOWN_MASK;
}
//  kbd->keydown = false;
//  kbd->keycode = AM_KEY_NONE;

