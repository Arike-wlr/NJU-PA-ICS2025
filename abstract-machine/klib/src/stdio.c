#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int get_num_len(unsigned int num, int base) {
  int len = 0;
  if (num == 0) return 1;
  
  while (num > 0) {
    len++;
    num /= base;
  }
  return len;
}

static int num2str(char *out, unsigned int num,int base){
  int len=get_num_len(num,base);
  int l=len;
  char num_char[32]; 

  if(num==0){
    *out = '0';
    return l;
  }

  int idx = len;
  while (num > 0) {
    int digit = num % base;
    if (digit < 10) {
      num_char[--idx] = digit + '0';  
    } else {
      num_char[--idx] = digit - 10 + 'a';  
    }
    num /= base;
  }

  for (int i = 0; i < l; i++) {
    *out = num_char[i];
    out++;
  }
  
  return l;
}

static void reverse(char *s,int len){
  char *start = s;
  char *end = s + len - 1;
  char tmp;
  while(start < end){
    tmp = *start;
    *start = *end;
    *end = tmp;
    start++;
    end--;
  }
}

static int str2num(int n,char *s,int base){
  assert(base <= 16);
  int i = 0,sign = n,bit;
  if(sign < 0) n=-n;
  do{
    bit = n % base;
    if(bit >= 10) s[i++] = 'a'+ bit - 10;
    else s[i++] = '0' + bit; 
  }while((n/=base)>0);
  if(sign < 0) s[i++] = '-';
  s[i] = '\0';
  reverse(s,i);
  return i;
}

int printf(const char *fmt, ...) {//printf是一个可变参数函数
  /*
  params:  
    1.fmt是format（格式）的缩写，它是一个格式字符串，用于指定输出的格式。
    2.  ...是C语言的可变参数语法，是语言本身的一部分
  return value:
    C标准规定printf函数应该返回成功输出的字符数
  */
  char buf[4096]; //建立一个缓冲区
  va_list ap;
  int n;
  va_start(ap,fmt);
  memset(buf,'\0',4096);      // 清空缓冲区
  n = vsprintf(buf,fmt,ap);   // 使用vsprintf格式化到缓冲区
  int i=0;
  while(buf[i]!='\0'){        // 逐个字符输出
    putch(buf[i]);
    i++;
  }
  va_end(ap);
  return n;           
}

// 应该在 vsprintf 中实现核心格式化逻辑，返回写入的字符数
int vsprintf(char *out, const char *fmt, va_list ap) {
  char *start = out; //记录输出字符串的起始位置
  //接下来开始遍历格式字符串fmt:
  for(const char *p = fmt; *p!='\0';p++){
    if(*p!='%'){ //格式内容，不是格式说明符，直接输出
      *out = *p;
      out++;
      continue;
    }
    else{
      p++; //跳过%，指向格式说明符
      switch(*p){
        case 'd':{ //整数,要把整数转换成字符串再输出
          int num = va_arg(ap, int); //获取下一个参数，期望类型是int
          if(num<0){ //处理负数
            *out = '-';
            out++;
            num = -num;
          }
          int n=num2str(out, num,10); //把整数转换成字符串并存储到out
          out+=n; //更新out指针位置
          break;
        }
        case 's':{ //字符串
          char *str = va_arg(ap, char*); //获取下一个参数，期望类型是char*
          while(*str!='\0'){ 
            *out = *str;
            out++; str++;
          }
          break;
        }
        case 'c':{ //单个字符
          char ch = (char)va_arg(ap, int); //char在可变参数中提升为int
          *out = ch;
          out++;
          break;
        }
        case '%':{ //输出百分号本身
          *out = '%';
          out++;
          break;
        }
        case 'x': {
          out += str2num(va_arg(ap,unsigned int), out, 16);
          break;
        }
        case 'p': {
          out += str2num(va_arg(ap,int), out, 16);
          break;
        }
        case 'u': { 
          unsigned int num = va_arg(ap, unsigned int);
          int n = num2str(out, num, 10);  
          out += n;
          break;
        }
        default: //遇到未知的格式说明符，直接输出它们（包括%）
          *out = '%'; out++; 
          *out = *p; out++;
          break;
      }
    }
  }
  *out = '\0'; //字符串结束符
  return out - start; //返回成功输出的字符数
}

// 输出到调用者提供的字符串out中，返回写入的字符数
int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
    int result;
    
    va_start(ap, fmt);
    result = vsprintf(out, fmt, ap);
    va_end(ap);
    
    return result;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  int result;
  
  va_start(ap, fmt);
  result = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  
  return result;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  if (n == 0) return 0;
  
  // 先格式化到临时缓冲区
  char temp_buf[4096];  // 或根据需要调整大小
  int len = vsprintf(temp_buf, fmt, ap);
  
  size_t copy_len = len;
  if (copy_len >= n) {
    copy_len = n - 1;  // 为 '\0' 留空间
  }
  
  memcpy(out, temp_buf, copy_len);
  out[copy_len] = '\0';
  
  return len;
}
#endif
