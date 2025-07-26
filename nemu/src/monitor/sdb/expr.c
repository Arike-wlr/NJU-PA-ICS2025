/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ,// equal
  TK_NUMBER, //number
  TK_HEX, //hexadecimal number(16.)
  TK_REG, //register
  TK_PLUS, // operator(+)
  TK_MINUS, // operator(-)
  TK_MUL, // operator(*)
  TK_DIV, // operator(/)
  TK_NEQ, // not equal
  /* TODO: Add more token types */
  TK_LPAREN,  // 括号 ( 
  TK_RPAREN,  // 括号 )
  TK_NEG,     // 负号
  TK_DEREF,   // 解引用
  //TK_ID,      //标识符（如变量名等）
};

static struct rule {
  const char *regex;
  int token_type;
} 

rules[] = {
  {" +", TK_NOTYPE},    // spaces(no meanings)
  {"\\(", TK_LPAREN},    // left parenthesis
  {"\\)", TK_RPAREN},    // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX}, // hexadecimal number(16.)
  {"[0-9]+", TK_NUMBER}, // number(10.)
  {"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REG}, // register
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"==", TK_EQ},        // equal相等
  {"!=", TK_NEQ},      // not equal不相等
  {"(^|[-+*/=,([[:space:]]])-", TK_NEG}, // negative sign
  {"(?<![[:alnum:]_\\)])[[:space:]]*\\*[[:space:]]*(?=[[:alnum:]_\\(\\$])",TK_DEREF},//derefence匹配，后面可以是字母（指针）数字（地址）$（寄存器）或左括号
  {"\\*", TK_MUL},         // multiply
  {"/", TK_DIV},           // divide
  {"\\+", TK_PLUS},         // plus
  {"\\-", TK_MINUS},         // minus
  //{"[a-zA-Z_]\\w*", TK_ID}, // identifier （不能是数字开头）
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) { // 匹配成功
        char *substr_start = e + position; // 匹配到的子串起始地址
        int substr_len = pmatch.rm_eo; // 匹配到的子串长度

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start); // 打印匹配信息

        position += substr_len;// 更新位置到匹配结束的位置

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE: break; // no type, do nothing
          case TK_EQ:
            tokens[nr_token].type = TK_EQ;
            strcpy(tokens[nr_token].str, "==");
            nr_token++;
            break;
          case TK_NEQ:
            tokens[nr_token].type = TK_NEQ;
            strcpy(tokens[nr_token].str, "!=");
            nr_token++;
            break;
          case TK_NUMBER:case TK_HEX://case TK_ID:
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // 因 strncpy 不保证终止符，确保字符串以'\0'结尾
            nr_token++;
            break;
          case TK_REG:
            tokens[nr_token].type = TK_REG;
            strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1); // 跳过'$',直接将名称传给isa_reg_str2val
            tokens[nr_token].str[substr_len - 1] = '\0'; // 同上，确保字符串以'\0'结尾
            nr_token++;
            break;
          case TK_LPAREN:case TK_RPAREN:case TK_NEG:case TK_DEREF:case TK_PLUS:case TK_MINUS:case TK_MUL:case TK_DIV:
            tokens[nr_token].type = rules[i].token_type; 
            tokens[nr_token].str[0] =substr_start[0] ; 
            tokens[nr_token].str[1] = '\0'; 
            nr_token++;
            break;
          default: 
          printf("Unknown token type %d at position %d\n", rules[i].token_type, position);
            return false; // unknown token
          TODO();
        }

        break;// 跳出 for 循环，继续处理下一个字符
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

//定义优先级：
static int precedence(int token_type) {
  switch (token_type) {
    case TK_NEG:
    case TK_DEREF: return 3;
    case TK_MUL: 
    case TK_DIV: return 2;
    case TK_PLUS:
    case TK_MINUS: return 1;
    case TK_EQ:
    case TK_NEQ: return 0;
    default: return -1;
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  if(nr_token == 0) {
    *success = false;
    return 0;
  }
  *success = true;
//这里使用逆波兰算法，先实现纯数学公式：
//需要操作符栈
Token op_stack[32];
int op_top = -1; // 栈顶指针
//需要操作数栈
word_t val_stack[32];
int val_top = -1; // 栈顶指针
for(int i =0; i<nr_token; i++) {
  Token curr_token = tokens[i];
  if(curr_token.type==TK_NUMBER || curr_token.type==TK_HEX){// 如果是数字或十六进制数，直接（将字符串转换为数值）存入数值栈中
    val_stack[++val_top]= strtol(curr_token.str, NULL, curr_token.type == TK_HEX ? 16 : 10);
  } 
  /*else if(curr_token.type==TK_ID) {// 如果是标识符?能根据变量名查找值吗？
  }*/
  else if(curr_token.type==TK_REG) {// 如果是寄存器，获取寄存器的值
    int reg_value = isa_reg_str2val(curr_token.str, success);
    if(!(*success)) {// 如果获取寄存器值失败
      printf("Invalid register name: %s\n", curr_token.str);
      return 0;
    }
    val_stack[++val_top] = reg_value; // 将寄存器值入栈
  }
  else if(curr_token.type==TK_LPAREN) {// 如果是左括号，直接整个存入操作符栈
    op_stack[++op_top] = curr_token;
  }
  else if(curr_token.type==TK_RPAREN) {// 如果是右括号，把栈中元素依次出栈并输出，直到遇到‘（’
    //这里或许可以直接计算？对确实可以，先实现一下单目双目操作符的逻辑
    while(op_top >= 0 && op_stack[op_top].type!=TK_LPAREN){
      Token op_token=op_stack[op_top--];
      
      if(op_token.type == TK_NEG || op_token.type == TK_DEREF) {//一目
        word_t value = val_stack[val_top--];
        if(op_token.type == TK_NEG) {
          val_stack[++val_top] = -value; 
        } 
        else if(op_token.type == TK_DEREF) {
          // 解引用操作
        }
      }
      else {//二目
        word_t right = val_stack[val_top--];
        word_t left = val_stack[val_top--];
        switch(op_token.type) {
          case TK_PLUS:
            val_stack[++val_top] = left + right;
            break;
          case TK_MINUS:
            val_stack[++val_top] = left - right;
            break;
          case TK_MUL:
            val_stack[++val_top] = left * right;
            break;
          case TK_DIV:
            if(right == 0) {
              printf("Error! The divisor cannot be zero!\n");
              *success = false;
              return 0;
            }
            val_stack[++val_top] = left / right;
            break;
          case TK_EQ:
            val_stack[++val_top] = (left == right) ? 1 : 0;
            break;
          case TK_NEQ: 
            val_stack[++val_top] = (left != right) ? 1 : 0;
            break;
          case TK_DEREF:
            vaddr_t addr = right; // value是地址
            if(addr < 0 ) {
              printf("Error! Address out of bounds: %u\n", addr);
              *success = false;
              return 0;
            }
            word_t deref_value = vaddr_read(addr, 4); // 读取地址处的值,4字节
            val_stack[++val_top] = deref_value;
            break;
          case TK_NEG:
            val_stack[++val_top] = -right; 
            break;
          default:
            printf("Unknown operator: %c\n", op_token.type);
            *success = false;
            return 0;
        }
      }//跟下面相同
    }
    op_top--; // 弹出左括号
  } 
  else if(curr_token.type==TK_PLUS || curr_token.type==TK_MINUS || curr_token.type==TK_MUL || curr_token.type==TK_DIV|| curr_token.type==TK_EQ || curr_token.type==TK_NEQ) {
    // 如果是双目操作符
    if (val_top < 1) { // 双目运算符需要至少2个操作数
      printf("Error: Not enough operands\n");
      *success = false;
      return 0;
    }
    while(op_top >= 0 && precedence(op_stack[op_top].type) >= precedence(curr_token.type)) {// 如果栈顶操作符优先级大于等于当前操作符，出栈并计算
     Token op_token = op_stack[op_top--];
     word_t right= val_stack[val_top--]; 
     word_t left = val_stack[val_top--]; 
     switch(op_token.type){
      case '+':
        val_stack[++val_top] = left + right;
        break;
      case '-':
        val_stack[++val_top] = left - right;
        break;
      case '*':
        val_stack[++val_top] = left * right; 
        break;
      case '/':
        if(right == 0) {
          printf("Error! The divisor cannot be zero!\n");
          *success = false;
          return 0;
        }
        val_stack[++val_top] = left / right; 
        break;
      case TK_EQ:
        val_stack[++val_top] = (left == right) ? 1 : 0;
        break;
      case TK_NEQ: 
        val_stack[++val_top] = (left != right) ? 1 : 0;
        break;
     }
    }
    op_stack[++op_top] = curr_token; // 最后将当前操作符入栈
  }
  else if(curr_token.type==TK_NEG || curr_token.type==TK_DEREF) {// 如果是一元操作符
    if (val_top < 0) { // 一目运算符需要至少1个操作数
      printf("Error: Not enough operands\n");
      *success = false;
      return 0;
    }
    word_t value = val_stack[val_top--]; // 获取栈顶值
    if(curr_token.type == TK_NEG) {
      val_stack[++val_top] = -value; // 负号操作
    } 
    else if(curr_token.type == TK_DEREF) {
      // 解引用操作,这是对什么东西操作的？我下次去看看，这里先空着
      vaddr_t addr = value; // value是地址
      if(addr < 0 ) {
        printf("Error! Address out of bounds: %u\n", addr);
        *success = false;
        return 0;
      }
      word_t deref_value = vaddr_read(addr, 4); // 读取地址处的值,4字节
      val_stack[++val_top] = deref_value; // 将解引用的值入栈
    }
  }
  else {
    printf("Unknown token type: %d\n", curr_token.type);
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
 }
 while(op_top >= 0) {// 如果操作符栈不为空，继续计算
   Token op_token = op_stack[op_top--];
   word_t right = val_stack[val_top--];
   word_t left = val_stack[val_top--];
   switch(op_token.type) {
     case TK_PLUS:
       val_stack[++val_top] = left + right;
       break;
     case TK_MINUS:
       val_stack[++val_top] = left - right;
       break;
     case TK_MUL:
       val_stack[++val_top] = left * right;
       break;
     case TK_DIV:
       if(right == 0) {
         printf("Error! The divisor cannot be zero!\n");
         *success = false;
         return 0;
       }
       val_stack[++val_top] = left / right;
       break;
     case TK_EQ:
       val_stack[++val_top] = (left == right) ? 1 : 0;
       break;
     case TK_NEQ: 
       val_stack[++val_top] = (left != right) ? 1 : 0;
       break;
    case TK_NEG:
       val_stack[++val_top] = -right; // 负号操作
       break;
    case TK_DEREF: 
      vaddr_t addr = right; // value是地址
      if(addr < 0 ) {
        printf("Error! Address out of bounds: %u\n", addr);
        *success = false;
        return 0;
      }
      word_t deref_value = vaddr_read(addr, 4); // 读取地址处的值,4字节
      val_stack[++val_top] = deref_value;
      
     default:
       printf("Unknown operator: %c\n", op_token.type);
       *success = false;
       return 0;
   }
  }
  // 表达式处理完成后检查栈状态
  if (val_top != 0) {
      printf("Error: Malformed expression\n");
      *success = false;
      return 0;
  }
  return val_stack[val_top]; // 返回栈顶的值，即表达式的结果
}
