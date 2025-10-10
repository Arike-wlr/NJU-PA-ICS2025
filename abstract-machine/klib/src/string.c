#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t len = 0;
  while (*s++) len++; // 更简洁的判断方法：increment len until \0.
  return len;
}

char *strcpy(char *dst, const char *src) {//将 src 指向的字符串（包括结尾的 \0 空字符）复制到 dest 指向的内存位置.
  char *ret = dst;
  while ((*dst++ = *src++) != '\0')
    ;
  return ret;
}

char *strncpy(char *dst, const char *src, size_t n) {//复制 n 个字符
  char *ret = dst;
  while (n && (*dst++ = *src++) != '\0') n--;
  while (n--) *dst++ = '\0'; // 如果 src 比 n 短，则在 dst 的结尾添加 \0，直到复制了 n 个字符为止.
  return ret;
}

char *strcat(char *dst, const char *src) {//将 src 字符串追加到 dest 字符串的末尾.
  char *ret = dst;
  while (*dst) dst++; // 移动到 dst 的结尾
  while ((*dst++ = *src++) != '\0')
    ;
  return ret;
}

int strcmp(const char *s1, const char *s2) {//逐个比较两个字符串的字符，直到遇到不同的字符或到达字符串的结尾.
  while (*s1 && *s2 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return (uint8_t)(*s1) - (uint8_t)(*s2);
}

int strncmp(const char *s1, const char *s2, size_t n) {//比较前 n 个字符
  while (n && *s1 && *s2 && (*s1 == *s2)) {
    s1++;
    s2++;
    n--;
  }
  if (n == 0) return 0;
  return (uint8_t)(*s1) - (uint8_t)(*s2);
}

void *memset(void *s, int c, size_t n) {//将 s 指向的内存块的前 n 个字节都设置为指定的 c.
  unsigned char *p = s;
  while (n--) {
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {//将 src 指向的内存块的 n 个字节复制到 dst 指向的内存位置
  unsigned char *d = dst;
  const unsigned char *s = src;
  if (d < s) {// 不重叠或 dst 在 src 之前 , 从前往后复制
    while (n--) {
      *d++ = *s++;
    }
  } 
  else {// 重叠且 dst 在 src 之后 , 从后往前复制
    d += n;
    s += n;
    while (n--) {
      *(--d) = *(--s);
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {//从 in 指向的内存位置复制 n 个字节到 out 指向的内存位置（跟上面一样？）
  assert(out != NULL || in != NULL);
  unsigned char *d = out;
  const unsigned char *s = in;
  while (n--) {
    *d++ = *s++;
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {//逐个比较两个内存块的前 n 个字节，直到遇到不同的字节或比较了 n 个字节.
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  while (n-- && (*p1 == *p2)) {
    p1++;
    p2++;
  }
  if (n == (size_t)-1) return 0; // 比较了 n 个字节且都相等
  return (int)(*p1 - *p2);
}

#endif
