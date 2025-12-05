#include <proc.h> // 进程控制块（PCB）相关
#include <elf.h>  // ELF 文件格式相关
#include <klib.h> // 内存操作函数，如 memset
#include <sys/types.h>
#include <fs.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

//size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t fs_open(const char *pathname, int flags, int mode);
size_t fs_read(int fd, void* buf, size_t len);
size_t fs_lseek(int fd, size_t offset, int whence);
size_t fs_close(int fd);

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  Elf_Phdr phdr;
  size_t fd;
  // Open the ELF file
  fd = fs_open(filename, 0, 0);
  assert(fd != -1);
  // Read the ELF header
  fs_read(fd, &ehdr, sizeof(ehdr));
  // Verify ELF magic number
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  assert(ehdr.e_machine == 243);
  // Load each program segment  
  for (int i = 0; i < ehdr.e_phnum; i++) {
    size_t offset = ehdr.e_phoff + i * ehdr.e_phentsize;
    fs_lseek(fd, offset, SEEK_SET);
    fs_read(fd, &phdr, sizeof(phdr));
    if (phdr.p_type == PT_LOAD) {
      // Load segment from file to memory
      size_t filesz = phdr.p_filesz;
      size_t memsz = phdr.p_memsz;
      if (filesz > 0) {
        fs_lseek(fd, phdr.p_offset, SEEK_SET);
        fs_read(fd, (void *)phdr.p_vaddr, filesz);
      }
      // Zero the memory region from p_filesz to p_memsz
      if (memsz > filesz) {
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, memsz - filesz);
      }
      Log("Loaded segment: vaddr=%u, filesz=%u, memsz=%u", phdr.p_vaddr, filesz, memsz);
    }
  }
  fs_close(fd);
  Log("Program entry point at %u", ehdr.e_entry);
  return ehdr.e_entry;

  // // read the ELF header
  // Elf_Ehdr ehdr;
  // ramdisk_read(&ehdr, 0, sizeof(ehdr));
  // // verify ELF magic number
  // assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  // assert(ehdr.e_machine == 243);
  // // load each program segment
  // Elf_Phdr phdr;
  // for (int i = 0; i < ehdr.e_phnum; i++) {
  //   size_t offset = ehdr.e_phoff + i * ehdr.e_phentsize;
  //   Log("Reading program header %d at offset %d", i, offset);
  //   ramdisk_read(&phdr,offset, sizeof(phdr));
  //   if (phdr.p_type == PT_LOAD) {
  //     // load segment from ramdisk to memory
  //     size_t filesz = phdr.p_filesz;
  //     size_t memsz = phdr.p_memsz;
  //     if (filesz > 0) ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, filesz);
  //     // zero the memory region from p_filesz to p_memsz
  //     if(memsz > filesz) memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, memsz - filesz);
  //     Log("Loaded segment: vaddr=%u, filesz=%u, memsz=%u", phdr.p_vaddr, filesz, memsz);
  //   }
  // }
  // // return the entry point of the program
  // Log("Program entry point at %u", ehdr.e_entry);
  // return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  //调用 loader()，直接跳转到用户程序入口.
  Log("naive_uload: loading %s", filename);
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) ();
}

