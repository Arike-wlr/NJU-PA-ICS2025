#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int get_num_len(int num){
  int len=0;
  if(num==0) return 1; 
  while(num){
    len++;
    num/=10;
  }
  return len;
}

int num2str(char *out, int num){
  int len=get_num_len(num);
  int l=len;
  char num_char[12]; 
  if(num==0){
    *out = '0';
    return l;
  }
  while (num) {
    num_char[--len] = (num % 10) + '0'; //取出最低位数字并转换为字符
    num /= 10; //去掉最低位数字
  }
  for(int i=0; i<l; i++) { //逐字符存储数字字符串
    *out = num_char[i];
    out++; 
  }
  return l;
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
          int n=num2str(out, num); //把整数转换成字符串并存储到out
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
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
