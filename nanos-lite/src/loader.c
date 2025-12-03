#include <proc.h> // 进程控制块（PCB）相关
#include <elf.h>  // ELF 文件格式相关
#include <klib.h> // 内存操作函数，如 memset
#include <sys/types.h>
#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

size_t ramdisk_read(void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  /*从 ramdisk 中加载 ELF 可执行文件到内存。
  pcb：进程控制块，包含进程的地址空间信息
  filename：要加载的 ELF 可执行文件的名称
  返回值：程序的入口地址
  */

  // read the ELF header
  Elf_Ehdr ehdr;
  ramdisk_read(&ehdr, 0, sizeof(ehdr));
  printf("ELF header dump:\n");
  printf("  e_ident[EI_MAG0-3]: %02x %02x %02x %02x\n",
         ehdr.e_ident[0], ehdr.e_ident[1], ehdr.e_ident[2], ehdr.e_ident[3]);
  printf("  e_type: %u\n", ehdr.e_type);
  printf("  e_machine: %u\n", ehdr.e_machine);
  printf("  e_version: %u\n", ehdr.e_version);
  printf("  e_entry: 0x%x\n", ehdr.e_entry);
  printf("  e_phoff: %u (should be 52)\n", ehdr.e_phoff);
  printf("  e_shoff: %u (should be 36848)\n", ehdr.e_shoff);
  printf("  e_ehsize: %u (should be 52)\n", ehdr.e_ehsize);
  printf("  e_phentsize: %u (should be 32)\n", ehdr.e_phentsize);
  printf("  e_phnum: %u (should be 4)\n", ehdr.e_phnum);
  printf("  e_shentsize: %u\n", ehdr.e_shentsize);
  printf("  e_shnum: %u\n", ehdr.e_shnum);

  // verify ELF magic number
  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
      ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
      ehdr.e_ident[EI_MAG3] != ELFMAG3) {
    panic("Invalid ELF magic number");
  }

  // load each program segment
  for (int i = 0; i < ehdr.e_phnum; i++) {
    Elf_Phdr phdr;
    off_t offset = ehdr.e_phoff + i * sizeof(phdr);
    Log("Reading program header %d at offset %d", i, offset);
    ramdisk_read(&phdr,offset, sizeof(phdr));

    if (phdr.p_type == PT_LOAD) {
      // load segment from ramdisk to memory
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      // zero the memory region from p_filesz to p_memsz
      memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
    }
  }
  // return the entry point of the program
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  //调用 loader()，直接跳转到用户程序入口.
  Log("naive_uload: loading %s", filename);
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

