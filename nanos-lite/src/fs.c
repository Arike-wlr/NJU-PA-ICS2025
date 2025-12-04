#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, invalid_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, invalid_write},
  [FD_FB]     = {"/dev/fb", 0, 0, invalid_read, invalid_write},
#include "files.h"
};

void init_fs() {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
  file_table[FD_FB].size = width * height * sizeof(uint32_t);
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    for (size_t i = 0; i < len; i++) {
      putch(((char *)buf)[i]);
    }
    return len;
  }
  Finfo *f = &file_table[fd];
  assert(f->write != NULL);
  return f->write(buf, 0, len);
}
