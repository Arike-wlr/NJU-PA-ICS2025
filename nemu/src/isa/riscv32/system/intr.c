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
//intr 是 interrupt（中断）的缩写
#include <isa.h>

#define MSTATUS_MIE 0x00000008
#define MSTATUS_MPIE 0x00000080

word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* Trigger an interrupt/exception with ``NO''（异常/中断的编号）.
    1. Record the current pc (``epc'') to the appropriate CSR.
    2. Set the pc to the corresponding interrupt/exception vector.
    3. Record ``NO'' to the appropriate CSR.
   * Then return the address of the interrupt/exception vector.
   */
  printf("intr NO = %x, epc = %x, mtvec = %x\n", NO, epc, cpu.csr.mtvec);
  cpu.csr.mepc = epc;
  cpu.csr.mcause = NO;
  if(cpu.csr.mstatus & MSTATUS_MIE){
    cpu.csr.mstatus |= MSTATUS_MPIE;
  }
  else{
    cpu.csr.mstatus &= (~MSTATUS_MPIE);
  }
  cpu.csr.mstatus &= (~MSTATUS_MIE);
  #ifdef CONFIG_ETRACE
  printf("[etrace] intr NO = %x, epc = %x, mtvec = %x\n", NO, epc, cpu.csr.mtvec);
  #endif
  return cpu.csr.mtvec;
}

word_t isa_query_intr() {
  return INTR_EMPTY;
}
