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
  {" +", TK_NOTYPE},    // spaces(no meanings) 0
  {"\\(", TK_LPAREN},    // left parenthesis 1
  {"\\)", TK_RPAREN},    // right parenthesis 2
  {"0[xX][0-9a-fA-F]+", TK_HEX}, // hexadecimal number(16.) 3
  {"[0-9]+", TK_NUMBER}, // number(10.) 4
  {"\\$(\\$)?\\w+", TK_REG}, // register 5
  {"==", TK_EQ},        // equal相等 6
  {"!=", TK_NEQ},      // not equal不相等 7
  {"\\*", TK_MUL},         // multiply 10
  {"/", TK_DIV},           // divide 11
  {"\\+", TK_PLUS},         // plus 12
  {"\\-", TK_MINUS},         // minus 13
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

static Token tokens[1000] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static void adjust_tokens() {
  for (int i = 0; i < nr_token; i++) {
    if (tokens[i].type == TK_MINUS) {

      if (i == 0 || 
          tokens[i-1].type == TK_PLUS ||
          tokens[i-1].type == TK_MINUS ||
          tokens[i-1].type == TK_MUL ||
          tokens[i-1].type == TK_DIV ||
          tokens[i-1].type == TK_EQ ||
          tokens[i-1].type == TK_NEQ ||
          tokens[i-1].type == TK_LPAREN ||
          tokens[i-1].type == TK_DEREF) {
        tokens[i].type = TK_NEG;  // 标记为负号
      }
    }
  
  else if(tokens[i].type == TK_MUL) {
      if (i == 0 || 
          tokens[i-1].type == TK_PLUS ||
          tokens[i-1].type == TK_MINUS ||
          tokens[i-1].type == TK_MUL ||
          tokens[i-1].type == TK_DIV ||
          tokens[i-1].type == TK_EQ ||
          tokens[i-1].type == TK_NEQ ||
          tokens[i-1].type == TK_LPAREN ||
          tokens[i-1].type == TK_NEG ) {
        tokens[i].type = TK_DEREF; // 标记为解引用
      }
    }
  }
}

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
        if (substr_start == NULL) {
          fprintf(stderr, "ERROR: substr_start is NULL\n");
          return -1;
        } 
        int substr_len = pmatch.rm_eo; // 匹配到的子串长度

        //Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",i, rules[i].regex, position, substr_len, substr_len, substr_start); // 打印匹配信息

        position += substr_len;// 更新位置到匹配结束的位置

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
            return false;
        }printf("Tokens[%d]: type=%d, str=%s\n", nr_token - 1, tokens[nr_token - 1].type, tokens[nr_token - 1].str);

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

static bool operation(Token *op_stack, int *op_top, sword_t *val_stack, int *val_top) {
  Token op_token=op_stack[(*op_top)--];//取操作符
  if(op_token.type==TK_NEG || op_token.type==TK_DEREF) {//一元操作符
    if(*val_top < 0) {
      printf("Error: Not enough operands for unary operator\n");
      return false;
    }
    sword_t value = val_stack[(*val_top)--]; // 获取栈顶值
    if(op_token.type == TK_NEG) {
      val_stack[++(*val_top)] = -value; // 负号操作
    } 
    else if(op_token.type == TK_DEREF) {
      vaddr_t addr = (word_t)value; // value是地址
      if(addr==0) {// 空指针检查
        printf("Error: Invalid address 0x%x\n", (word_t)addr);
        return false;
      }
      if (addr % 4 != 0) {// 地址对齐检查
        printf("Error: Address 0x%x is not 4-byte aligned\n", (word_t)addr);
        return false;
      }
      word_t deref = vaddr_read(addr, 4); // 读取地址处的值,4字节
      if(deref == (word_t)-1) {
        printf("Error: Invalid memory access at address 0x%x\n", (word_t)addr);
        return false;
      }
      sword_t deref_value = (sword_t)deref;
      val_stack[++(*val_top)] = deref_value; // 将解引用的值入栈
    }
  }
  else {
    if(*val_top < 1) {
      printf("Error: Not enough operands for binary operator\n");
      return false;
    }
    sword_t right= val_stack[(*val_top)--]; //取操作数1
    sword_t left = val_stack[(*val_top)--];//取操作数2
    switch(op_token.type) {
     case TK_PLUS:
       val_stack[++(*val_top)] = left + right;
       break;
     case TK_MINUS:
       val_stack[++(*val_top)] = left - right;
       break;
     case TK_MUL:
       val_stack[++(*val_top)] = left * right;
       break;
     case TK_DIV:
       if(right == 0) {
         printf("Error! The divisor cannot be zero!\n");
         return false;
       }
       val_stack[++(*val_top)] = left / right;
       break;
     case TK_EQ:
       val_stack[++(*val_top)] = (left == right) ? 1 : 0;
       break;
     case TK_NEQ: 
       val_stack[++(*val_top)] = (left != right) ? 1 : 0;
       break;
     default:
       printf("Unknown operator: %c\n", op_token.type);
       return false;
   }
  }
  return true; // 成功执行操作
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
  adjust_tokens(); // 调整tokens中的负号和解引用符号
  *success = true;
  //这里使用逆波兰表达式的算法
  Token op_stack[1000];//需要操作符栈
  int op_top = -1; // 栈顶指针
  sword_t val_stack[1000];//需要操作数栈
  int val_top = -1; // 栈顶指针
  for(int i =0; i<nr_token; i++) {
    Token curr_token = tokens[i];
    if(curr_token.type==TK_NUMBER || curr_token.type==TK_HEX){// 如果是数字或十六进制数，直接（将字符串转换为数值）存入数值栈中
      val_stack[++val_top]= (sword_t)strtol(curr_token.str, NULL, curr_token.type == TK_HEX ? 16 : 10);
    } 
    else if(curr_token.type==TK_REG) {// 如果是寄存器，获取寄存器的值
      sword_t reg_value = (sword_t)isa_reg_str2val(curr_token.str, success);
      if(!(*success)) {// 如果获取寄存器值失败
        printf("Invalid register name: %s\n", curr_token.str);
        return 0;
      }
      val_stack[++val_top] = reg_value; // 将寄存器值入栈
    }
    else if(curr_token.type==TK_LPAREN) {// 如果是左括号，直接整个存入操作符栈
      op_stack[++op_top] = curr_token;
    }
    else if(curr_token.type==TK_RPAREN) {// 如果是右括号，把栈中元素依次出栈并输出，直到遇到‘(’
      while(op_top >= 0 && op_stack[op_top].type!=TK_LPAREN){
        *success=operation(op_stack, &op_top, val_stack, &val_top); // 执行操作
        if(!(*success)) {
          return 0; 
        }
      }
      if(op_top >= 0 && op_stack[op_top].type == TK_LPAREN) op_top--; // 弹出左括号
      else {
        printf("Error: Mismatched parentheses\n");
        *success = false;
        return 0; // 左右括号匹配检查
      }
    } 
    else if(curr_token.type==TK_PLUS || curr_token.type==TK_MINUS ||
            curr_token.type==TK_MUL  || curr_token.type==TK_DIV   || 
            curr_token.type==TK_EQ   || curr_token.type==TK_NEQ   ||
            curr_token.type==TK_NEG  || curr_token.type==TK_DEREF ) {// 如果是操作符
      while(op_top >= 0 && precedence(op_stack[op_top].type) >= precedence(curr_token.type)) {// 如果栈顶操作符优先级大于等于当前操作符，出栈并计算
        *success = operation(op_stack, &op_top, val_stack, &val_top); 
        if(!(*success)) {
          return 0; 
        }
      }
      op_stack[++op_top] = curr_token; // 最后将当前操作符入栈
    }
    else {
      printf("Unknown token type: %d\n", curr_token.type);
      *success = false;
      return 0;
    }
  }
  while(op_top >= 0) {// 如果操作符栈不为空，继续计算
    *success = operation(op_stack, &op_top, val_stack, &val_top);
    if(!(*success)) { 
      return 0;
    }
  }
    // 表达式处理完成后检查栈状态
  if (val_top != 0) {
    printf("Error: Malformed expression\n");
    *success = false;
    return 0;
  }
  return (word_t)val_stack[val_top]; // 返回栈顶的值，即表达式的结果
}
