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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef struct{
  word_t mstatus; //机器状态寄存器
  word_t misa; //机器ISA寄存器
  word_t mie; //机器中断使能寄存器
  word_t mtvec; //机器中断向量基地址寄存器
  vaddr_t mepc; //机器异常程序计数器
  word_t mcause; //机器异常原因寄存器
  word_t mtval; //机器异常值寄存器
  word_t mip; //机器中断挂起寄存器
  word_t mscratch; //机器临时寄存器
  word_t satp; //地址转换和保护寄存器
} riscv32_CSR_state;

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;
  #ifdef CONFIG_IRINGBUF
  struct{
    vaddr_t pc[CONFIG_IRINGBUF_SIZE]; //指令的虚拟地址
    uint32_t inst[CONFIG_IRINGBUF_SIZE]; //指令的二进制编码
    int ilen[CONFIG_IRINGBUF_SIZE]; //指令长度,用于反汇编
    int head; //环形缓冲区头指针
    int count; //
    bool enabled; //是否启用指令环形缓冲区  
  }ringbuf;
  #endif
  riscv32_CSR_state csr;
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state);

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
