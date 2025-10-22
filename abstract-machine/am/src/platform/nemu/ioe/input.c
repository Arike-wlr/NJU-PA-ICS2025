#include <am.h>
#include <nemu.h>

#define KEYDOWN_MASK 0x8000 // 从键盘扫描码或按键状态值中提取按键按下/释放的信息,2进制的第15位为1表示按下,为0表示释放

void __am_input_keybrd(AM_INPUT_KEYBRD_T *kbd) {
  int k = inw(KBD_ADDR);
  kbd->keydown = (k & KEYDOWN_MASK ? true : false);
  kbd->keycode = k & ~KEYDOWN_MASK;
}
//  

