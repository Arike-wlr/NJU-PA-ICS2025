#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);
#define NR_FILES sizeof(file_table)/sizeof(Finfo)
typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  size_t open_offset;
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR,FD_EVENT, FD_FBINFO, FD_FBDEV};
size_t ramdisk_read(void *buf, size_t offset, size_t len);
size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t serial_write(const void *buf, size_t offset, size_t len);
size_t events_read(void *buf, size_t offset, size_t len);
size_t dispinfo_read(void *buf, size_t offset, size_t len);
size_t fb_write(const void *buf, size_t offset, size_t len);

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
  [FD_STDIN]  = {"stdin", 0, 0,0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0,0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, 0,invalid_read, serial_write},
  [FD_EVENT]  = {"/dev/events",0,0,0,events_read,   invalid_write},
  [FD_FBINFO] = {"/dev/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
  [FD_FBDEV]  = {"/dev/fb", 0, 0, 0,invalid_read, fb_write},
#include "files.h"
};

void init_fs() {
  AM_GPU_CONFIG_T ev = io_read(AM_GPU_CONFIG);
  int width = ev.width;
  int height = ev.height;
  file_table[FD_FBDEV].size = width * height * sizeof(uint32_t);
}

size_t fs_open(const char *pathname, int flags, int mode) {
  for (int idx=0; idx<NR_FILES; idx++) {
    if (strcmp(file_table[idx].name , pathname) == 0) {
      file_table[idx].open_offset = 0;
      file_table[idx].read = ramdisk_read;
      file_table[idx].write = ramdisk_write;
      return idx;
    }
  }
  printf("cannot find requested file\n");
  assert(0);
  return 2;
}

size_t fs_read(int fd, void* buf, size_t len) {
  size_t off; 
  assert(fd>2 && fd<NR_FILES);
  if (file_table[fd].open_offset + len > file_table[fd].size) { panic("file operation exceed max size"); }

  off = file_table[fd].disk_offset + file_table[fd].open_offset;
  file_table[fd].read(buf, off, len);
  file_table[fd].open_offset += len;
  return len;
}

size_t fs_write(int fd, const void* buf, size_t len) {
  size_t off; 
  assert(fd>2 && fd<NR_FILES);
  if (file_table[fd].open_offset + len > file_table[fd].size) { panic("file operation exceed max size"); }

  off = file_table[fd].disk_offset + file_table[fd].open_offset;
  file_table[fd].write(buf, off, len);
  file_table[fd].open_offset += len;
  return len;
}

size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd>2 && fd<NR_FILES);

  if (whence == SEEK_SET) {
    if (offset > file_table[fd].size || offset < 0) { panic("file operation exceed max size"); }
    file_table[fd].open_offset = offset;
  }
  else if (whence == SEEK_CUR) {
    size_t cur = file_table[fd].open_offset;
    if (offset+cur > file_table[fd].size || offset+cur < 0) { panic("file operation exceed max size"); }
    file_table[fd].open_offset += offset;
  }
  else if (whence == SEEK_END) {
    file_table[fd].open_offset = file_table[fd].size + offset;
  }
    
  return file_table[fd].open_offset;
}

size_t fs_close(int fd) {
  assert(fd>2 && fd<NR_FILES);

  file_table[fd].open_offset = 0;
  file_table[fd].read = NULL;
  file_table[fd].write = NULL;
  return 0;
}
