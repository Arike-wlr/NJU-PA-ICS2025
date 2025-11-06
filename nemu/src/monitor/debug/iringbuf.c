#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

#ifdef CONFIG_IRINGBUF
void init_iringbuf(void){
  cpu.ringbuf.head = 0;
  cpu.ringbuf.count = 0;
  cpu.ringbuf.enabled = true;
  for (int i = 0; i < CONFIG_IRINGBUF_SIZE; i++) {
    cpu.ringbuf.pc[i] = 0;
    cpu.ringbuf.inst[i] = 0;
    cpu.ringbuf.ilen[i] = 0;
  }
}

void ringbuf_dump(void){
  if (cpu.ringbuf.count == 0) {
    printf("Instruction ring buffer is empty\n");
    return;
  }
  printf("Instruction ring buffer (last %d instructions):\n", cpu.ringbuf.count);
  int start = (cpu.ringbuf.head - cpu.ringbuf.count + CONFIG_IRINGBUF_SIZE) % CONFIG_IRINGBUF_SIZE;
for (int i = 0; i < cpu.ringbuf.count; i++) {
    int idx = (start + i) % CONFIG_IRINGBUF_SIZE;
    const char *marker = (i == cpu.ringbuf.count - 1) ? "-->" : "   ";
    
    printf("%s " FMT_WORD ": ", marker, cpu.ringbuf.pc[idx]);
    
    uint8_t *bytes = (uint8_t *)&cpu.ringbuf.inst[idx];
    for (int j = cpu.ringbuf.ilen[idx] - 1; j >= 0; j--) {
      printf("%02x", bytes[j]);
      if (j > 0) printf(" ");
    }
    
    int space_len = 12 - cpu.ringbuf.ilen[idx] * 3;
    for (int j = 0; j < space_len; j++) printf(" ");
    
    char disasm_buf[64];
    void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
    disassemble(disasm_buf, sizeof(disasm_buf), 
                cpu.ringbuf.pc[idx], bytes, cpu.ringbuf.ilen[idx]);
    printf("%s\n", disasm_buf);
  }
}

void ringbuf_push(vaddr_t pc, uint32_t inst, int ilen) {
  if (!cpu.ringbuf.enabled) return;
  
  int idx = cpu.ringbuf.head;
  cpu.ringbuf.pc[idx] = pc;
  cpu.ringbuf.inst[idx] = inst;
  cpu.ringbuf.ilen[idx] = ilen;
  
  cpu.ringbuf.head = (idx + 1) % CONFIG_IRINGBUF_SIZE;
  if (cpu.ringbuf.count < CONFIG_IRINGBUF_SIZE) {
    cpu.ringbuf.count++;
  }
}
#endif