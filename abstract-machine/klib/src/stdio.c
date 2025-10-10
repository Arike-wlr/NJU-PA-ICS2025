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

void print_num2str(int num){
  int len=get_num_len(num);
  char num_char[12]; 
  if(num==0){
    putch('0');
    return 1;
  }
  while (num) {
    num_char[--len] = (num % 10) + '0'; //取出最低位数字并转换为字符
    num /= 10; //去掉最低位数字
  }
  for(int i=0; i<len; i++) //逐字符输出数字字符串
    putch(num_char[i]); 
}

int printf(const char *fmt, ...) {//printf是一个可变参数函数
  /*
  params:  
    1.fmt是format（格式）的缩写，它是一个格式字符串，用于指定输出的格式。
    2.  ...是C语言的可变参数语法，是语言本身的一部分
  return value:
    C标准规定printf函数应该返回成功输出的字符数
  */
  int ret = 0; //返回值，成功输出的字符数
  va_list args; //va_list 是一个类型，用于声明一个"可变参数列表"变量
  va_start(args, fmt); //va_start宏初始化args，使其指向第一个可变参数,fmt是可变参数之前的最后一个固定参数
  //接下来开始遍历格式字符串fmt:
  for(const char *p = fmt; *p!='\0';p++){
    if(*p!='%'){ //格式内容，不是格式说明符，直接输出
      putch(*p); //putch函数用于输出单个字符
      ret++;
      continue;
    }
    else{
      p++; //跳过%，指向格式说明符
      switch(*p){
        case 'd':{ //整数,要把整数转换成字符串再输出
          int num = va_arg(args, int); //获取下一个参数，期望类型是int
          if(num<0){ //处理负数
            putch('-');
            ret++;
            num = -num;
          }
          print_num2str(num); //输出数字字符串
          ret+=get_num_len(num); //更新输出字符数
          break;
        }
        case 's':{ //字符串
          char *str = va_arg(args, char*); //获取下一个参数，期望类型是char*
          while(*str!='\0'){ //逐字符输出字符串
            putch(*str);
            ret++;
            str++;
          }
          break;
        }
        case 'c':{ //单个字符
          char ch = (char)va_arg(args, int); //char在可变参数中提升为int
          putch(ch);
          ret++;
          break;
        }
        case '%':{ //输出百分号本身
          putch('%');
          ret++;
          break;
        }
        default: //遇到未知的格式说明符，直接输出它们（包括%）
          putch('%');
          putch(*p);
          ret+=2;
          break;
      }
    }
  }
  va_end(args); //va_end宏用于清理args，通常在处理完可变参数后调用
}

int vsprintf(char *out, const char *fmt, va_list ap) {

}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
