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

enum {
  TK_NOTYPE = 256, TK_EQ,// equal
  TK_NUMBER, //number
  TK_HEX, //hexadecimal number(16.)
  TK_REG, //register
  '+', // operator(+)
  '-', // operator(-)
  '*', // operator(*)
  '/', // operator(/)
  TK_NEQ, // not equal
  /* TODO: Add more token types */
  TK_LPAREN,  // 括号 ( )
  TK_RPAREN,  // 括号 ( )
  TK_NEG,     // 负号
  TK_DEREF,   // 解引用
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
  {"^-|(?<=[+\\-*/=,( ])-", TK_NEG}, // negative sign
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"^-|(?<=[+\\-*/=,( ])-",TK_NEG},
  {"\\+", '+'},         // plus
  {"\\-", '-'},         // minus
  {"\\*",TK_DEREF},     //derefence
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
          case TK_NUMBER:
            tokens[nr_token].type = TK_NUMBER;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0'; // 确保字符串以'\0'结尾
            nr_token++;
            break;
          case TK_HEX:
            tokens[nr_token].type = TK_HEX;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;
          case TK_REG:
            tokens[nr_token].type = TK_REG;
            strncpy(tokens[nr_token].str, substr_start + 1, substr_len - 1); // 跳过'$'
            tokens[nr_token].str[substr_len - 1] = '\0'; // 确保字符串以'\0'结尾
            nr_token++;
            break;
          case TK_LPAREN:
            tokens[nr_token].type = TK_LPAREN;
            tokens[nr_token].str[0] = '('; 
            tokens[nr_token].str[1] = '\0'; 
            nr_token++;
            break;
          case TK_RPAREN:
            tokens[nr_token].type = TK_RPAREN;
            tokens[nr_token].str[0] = ')'; 
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case TK_NEG:
            tokens[nr_token].type = TK_NEG;
            tokens[nr_token].str[0] = '-';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case TK_DEREF:
            tokens[nr_token].type = TK_DEREF;
            tokens[nr_token].str[0] = '*';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case '+':
            tokens[nr_token].type = '+';
            tokens[nr_token].str[0] = '+';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case '-':
            tokens[nr_token].type = '-';
            tokens[nr_token].str[0] = '-';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case '*':
            tokens[nr_token].type = '*';
            tokens[nr_token].str[0] = '*';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case '/':
            tokens[nr_token].type = '/';
            tokens[nr_token].str[0] = '/';
            tokens[nr_token].str[1] = '\0';
            nr_token++;
            break;
          case TK_NEQ:
            tokens[nr_token].type = TK_NEQ;
            strcpy(tokens[nr_token].str, "!=");
            nr_token++;
            break;

          default: 
          printf("Unknown token type %d at position %d\n", rules[i].token_type, position);
            return false; // unknown token
          TODO();
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
