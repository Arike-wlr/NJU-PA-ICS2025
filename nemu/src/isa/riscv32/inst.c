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

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_R, TYPE_I, TYPE_S, TYPE_B, TYPE_U, TYPE_J,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)
#define immB() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 12) | (BITS(i, 7, 7) << 11) | (BITS(i, 30, 25) << 5) | (BITS(i, 11, 8) << 1); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | (BITS(i, 19, 12) << 12) | (BITS(i, 20, 20) << 11) | (BITS(i, 30, 21) << 1); } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_R: src1R(); src2R();         break;
    case TYPE_I: src1R();          immI(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}

static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
// s 是一个指向 Decode 结构体的指针
  INSTPAT_START();
  // R型指令
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add     , R, R(rd) = src1 + src2); // Add（加法），目标寄存器=源寄存器1+源寄存器2
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub     , R, R(rd) = src1 - src2);
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul     , R, R(rd) = src1 * src2); // Multiply（乘法），目标寄存器=源寄存器1*源寄存器2
  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", div     , R, R(rd) = (int32_t)src1 / (int32_t)src2); // Divide（除法），目标寄存器=源寄存器1/源寄存器2（有符号数除法）
  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor     , R, R(rd) = src1 ^ src2); // Xor（按位异或），目标寄存器=源寄存器1^源寄存器2
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll     , R, R(rd) = src1 << (src2 & 0x1f)); // Shift Left Logical（逻辑左移），目标寄存器=源寄存器1<<源寄存器2（高位补0）
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl     , R, R(rd) = src1 >> (src2 & 0x1f)); // Shift Right Logical（逻辑右移），目标寄存器=源寄存器1>>源寄存器2（高位补0）
  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra     , R, R(rd) = (int32_t)src1 >> (src2 & 0x1f)); // Shift Right Arithmetic（算术右移），目标寄存器=源寄存器1>>源寄存器2（高位补符号位）
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt     , R, R(rd) = (int32_t)src1 < (int32_t)src2); // Set Less Than（小于则置位），如果源寄存器1的值小于源寄存器2的值，则目标寄存器置1，否则置0（有符号数比较）
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu    , R, R(rd) = src1 < src2); // Set Less Than Unsigned（无符号数小于则置位），如果源寄存器1的值小于源寄存器2的值，则目标寄存器置1，否则置0（无符号数比较）
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and     , R, R(rd) = src1 & src2); // And（按位与），目标寄存器=源寄存器1&源寄存器2
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or      , R, R(rd) = src1 | src2); // Or（按位或），目标寄存器=源寄存器1|源寄存器2
  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", mulhu   , R, R(rd) = ((uint64_t)src1 * (uint64_t)src2) >> 32); // Multiply High Unsigned（无符号数乘法高位），目标寄存器=源寄存器1*源寄存器2的高32位
  // I型指令
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu     , I, R(rd) = Mr(src1 + imm, 1)); //Load Byte Unsigned（无符号字节加载）
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr    , I, R(rd) = s->snpc, s->dnpc = (src1 + imm) & ~1); // Jump and Link Register（寄存器跳转并链接），把下一条指令地址写入目标寄存器，然后跳转到目标地址。
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi    , I, R(rd) = src1 + imm); // Add Immediate（加立即数），目标寄存器=源寄存器+立即数
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori     , I, R(rd) = src1 | imm); // Or Immediate（按位或立即数），目标寄存器=源寄存器|立即数
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori    , I, R(rd) = src1 ^ imm); // Xor Immediate（按位异或立即数），目标寄存器=源寄存器^立即数
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb      , I, R(rd) = (int32_t)(int8_t)Mr(src1 + imm, 1)); // Load Byte（加载字节），把内存中的值加载到目标寄存器中，进行符号扩展
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh      , I, R(rd) = (int32_t)(int16_t)Mr(src1 + imm, 2)); // Load Halfword（加载半字），把内存中的值加载到目标寄存器中，进行符号扩展
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw      , I, R(rd) = Mr(src1 + imm, 4)); // Load Word（加载字），把内存中的值加载到目标寄存器中
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu     , I, R(rd) = Mr(src1 + imm, 2)); // Load Halfword Unsigned（无符号半字加载），把内存中的值加载到目标寄存器中，不进行符号扩展
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi    , I, R(rd) = src1 & imm); // And Immediate（按位与立即数），目标寄存器=源寄存器&立即数
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti    , I, R(rd) = (int32_t)src1 < (int32_t)imm); // Set Less Than Immediate（小于则置位立即数），如果源寄存器的值小于立即数，则目标寄存器置1，否则置0（有符号数比较）
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu   , I, R(rd) = src1 < imm); // Set Less Than Immediate Unsigned（无符号数小于则置位立即数），如果源寄存器的值小于立即数，则目标寄存器置1，否则置0（无符号数比较）
  INSTPAT("0000000 ????? ????? 001 ????? 00100 11", slli    , I, R(rd) = src1 << (imm & 0x1f)); // Shift Left Logical Immediate（逻辑左移立即数），目标寄存器=源寄存器<<立即数
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli    , I, R(rd) = src1 >> (imm & 0x1f)); // Shift Right Logical Immediate（逻辑右移立即数），目标寄存器=源寄存器>>立即数（高位补0）
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai    , I, R(rd) = (int32_t)src1 >> (imm & 0x1f)); // Shift Right Arithmetic Immediate（算术右移立即数），目标寄存器=源寄存器>>立即数（高位补符号位）
  // S型指令
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb      , S, Mw(src1 + imm, 1, src2)); //Store Byte（存储字节）
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh      , S, Mw(src1 + imm, 2, src2)); // Store Halfword（存储半字），把源寄存器的值存储到内存中
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw      , S, Mw(src1 + imm, 4, src2)); // Store Word（存储字），把源寄存器的值存储到内存中
  // B型指令
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq     , B, if (src1 == src2) s->dnpc = s->pc + imm); // Branch if Equal（等于则分支），如果源寄存器1的值等于源寄存器2的值，则跳转到目标地址
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne     , B, if (src1 != src2) s->dnpc = s->pc + imm); // Branch if Not Equal（不等则分支），如果源寄存器1的值不等于源寄存器2的值，则跳转到目标地址
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt     , B, if ((int32_t)src1 < (int32_t)src2) s->dnpc = s->pc + imm); // Branch if Less Than（小于则分支），如果源寄存器1的值小于源寄存器2的值，则跳转到目标地址（有符号数比较）
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge     , B, if ((int32_t)src1 >= (int32_t)src2) s->dnpc = s->pc + imm); // Branch if Greater Than or Equal（大于等于则分支），如果源寄存器1的值大于等于源寄存器2的值，则跳转到目标地址（有符号数比较）
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu    , B, if (src1 < src2) s->dnpc = s->pc + imm); // Branch if Less Than Unsigned（无符号数小于则分支），如果源寄存器1的值小于源寄存器2的值，则跳转到目标地址（无符号数比较）
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu    , B, if (src1 >= src2) s->dnpc = s->pc + imm); // Branch if Greater Than or Equal Unsigned（无符号数大于等于则分支），如果源寄存器1的值大于等于源寄存器2的值，则跳转到目标地址（无符号数比较）
  // U型指令
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui     , U, R(rd) = imm); // Load Upper Immediate（加载高位立即数），把立即数的高20位加载到目标寄存器的高20位，低12位置0
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc   , U, R(rd) = s->pc + imm); // Add Upper Immediate to PC
  // J型指令
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal     , J, R(rd) = s->snpc, s->dnpc = s->pc + imm); // Jump and Link（跳转并链接），把下一条指令地址写入目标寄存器，然后跳转到目标地址。
  // N型指令
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak  , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0,Environment Break（环境断点）
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv     , N, INV(s->pc)); // invalid instruction
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

/*
寄存器别名：
R(10) 就是 a0 寄存器
R(11) 是 a1 寄存器
R(12) 是 a2 寄存器
R(13) 是 a3 寄存器
*/

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
