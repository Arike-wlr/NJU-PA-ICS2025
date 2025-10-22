#include <amtest.h>

void simple_test() {
  int width = io_read(AM_GPU_CONFIG).width;
  int height = io_read(AM_GPU_CONFIG).height;
  
  printf("Screen resolution: %dx%d\n", width, height);
  
  // 测试红色
  uint32_t red_buf[1] = {0xFF0000};
  printf("Testing RED...\n");
  io_write(AM_GPU_FBDRAW, 0, 0, red_buf, width, height, true);
  
  // 等待3秒
  unsigned long start = io_read(AM_TIMER_UPTIME).us / 1000;
  while (io_read(AM_TIMER_UPTIME).us / 1000 - start < 3000) {}
  
  // 测试绿色
  uint32_t green_buf[1] = {0x00FF00};
  printf("Testing GREEN...\n");
  io_write(AM_GPU_FBDRAW, 0, 0, green_buf, width, height, true);
  
  printf("Test completed - screen should be GREEN\n");
}