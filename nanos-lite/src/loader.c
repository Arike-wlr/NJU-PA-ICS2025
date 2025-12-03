#include <proc.h> // 进程控制块（PCB）相关
#include <elf.h>  // ELF 文件格式相关
#include <klib.h> // 内存操作函数，如 memset
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
  if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
      ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
      ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
      ehdr.e_ident[EI_MAG3] != ELFMAG3) {
    panic("Invalid ELF magic number");
  }
  // load each program segment
  for (int i = 0; i < ehdr.e_phnum; i++) {
    Elf_Phdr phdr;
    ramdisk_read(&phdr, ehdr.e_phoff + i * sizeof(phdr), sizeof(phdr));

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

