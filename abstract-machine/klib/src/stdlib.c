#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

static char *addr = NULL;
static bool initialized = false;

void *malloc(size_t size) { //size是分配的字节数
  // Initialize the heap.
  if (!initialized) {
    addr = (char*)ROUNDUP((uintptr_t)heap.start, 8);
    initialized = true;
    printf("分配器初始化完成，起始地址: %p\n", addr);
  }

  size = ROUNDUP(size, 8);// 对齐大小  
  if (addr + size <= (char*)heap.end) {
    char *start = addr;
    addr += size;
    //把之前分配的区域清零
    for(char* p=start; p<addr; p++) {
      *p = 0;
    }
    //返回分配的内存首地址
    return (void*) start;
  }
  printf("内存分配失败，剩余空间不足，申请大小: %zu 字节\n", size);
  return NULL;
}

void free(void *ptr) {
}

#endif
