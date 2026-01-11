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
  file_table[FD_FBDEV].size = (int)ev.width * (int)ev.height * sizeof(uint32_t);
}

size_t fs_open(const char *pathname, int flags, int mode) {
  for (int i=0; i<NR_FILES; i++) {
    if (strcmp(file_table[i].name , pathname) == 0) {
      file_table[i].open_offset = 0;

      if (!file_table[i].read && !file_table[i].write) {
        file_table[i].read = ramdisk_read;
        file_table[i].write = ramdisk_write;
      }
      return i;
    }
  }
  printf("cannot find requested file %s\n", pathname);
  return 2;
}

size_t fs_read(int fd, void* buf, size_t len) {
  
  if(fd == FD_STDIN) return 0;
  else if (fd == FD_EVENT || fd == FD_FBINFO) {
    return file_table[fd].read(buf, 0, len);
  }
  else if (fd== FD_STDOUT || fd == FD_STDERR) {
    Log("fs_read: attempt to read from write-only fd %d (%s)", 
        fd, file_table[fd].name);
    return -1; 
  }
  else{
    if (file_table[fd].open_offset + len > file_table[fd].size) {len= file_table[fd].size - file_table[fd].open_offset;}

    size_t off; 
    off = file_table[fd].disk_offset + file_table[fd].open_offset;
    file_table[fd].read(buf, off, len);
    file_table[fd].open_offset += len;
    return len;
  }
}

size_t fs_write(int fd, const void* buf, size_t len) {
  if (fd==FD_STDIN || fd==FD_EVENT || fd==FD_FBINFO) {
    Log("fs_write: attempt to write to read-only fd %d (%s)", 
        fd, file_table[fd].name);
    return -1;
  }
  else if (fd== FD_STDOUT || fd== FD_STDERR) {
    return file_table[fd].write(buf,0,len);
  }
  else{
    if (file_table[fd].open_offset + len > file_table[fd].size) len = file_table[fd].size - file_table[fd].open_offset;
    
    size_t off;
    off = file_table[fd].disk_offset + file_table[fd].open_offset;
    file_table[fd].write(buf, off, len);
    file_table[fd].open_offset += len;
    return len;
  }
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
  if (fd==FD_STDIN || fd==FD_STDOUT || fd==FD_STDERR) {
    return 0;
  }
  assert(fd>2 && fd<NR_FILES);
  file_table[fd].open_offset = 0;
  return 0;
}
