#include <am.h>
#include <nemu.h>
#include <stdio.h>

#define SYNC_ADDR (VGACTL_ADDR + 4) //同步寄存器的地址

void __am_gpu_init() {
  int i;
  uint32_t v = inl(VGACTL_ADDR);
  uint32_t w = v >> 16;
  uint32_t h = v & 0xFFFF;
  printf("Detected resolution: %dx%d\n", w, h);
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  for (i = 0; i < w * h; i ++) fb[i] = i;
  outl(SYNC_ADDR, 1);
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  uint32_t v = inl(VGACTL_ADDR);
  uint32_t w = v >> 16;
  uint32_t h = v & 0xFFFF;
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = w, .height = h,
    .vmemsz = 0
  };
}

void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  if(!ctl->pixels){
    if (ctl->sync) {
      outl(SYNC_ADDR, 1);
    }
    return;
  }
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  uint32_t v = inl(VGACTL_ADDR);
  uint32_t screen_w = v >> 16;
  uint32_t screen_h = v & 0xFFFF;

  uint32_t *pixels = (uint32_t *)ctl->pixels;
  for (int y = 0; y < ctl->h; y ++) {
    if (ctl->y + y >= screen_h) break;
    for (int x = 0; x < ctl->w; x ++) {
      if (ctl->x + x >= screen_w) break;
      fb[(ctl->y + y) * screen_w + (ctl->x + x)] = pixels [y * ctl->w + x];
    }
  }

  if (ctl->sync) {
      outl(SYNC_ADDR, 1);
    }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
