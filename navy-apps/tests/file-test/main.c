#include <stdio.h>
#include <assert.h>

int main() {
  FILE *fp = fopen("/share/files/num", "r+");
  assert(fp);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  printf("size: %ld\n", size);
  assert(size == 6000);

  fseek(fp, 500 * 5, SEEK_SET);
  int i, n;
  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    
    //assert(n == i + 1);
  }printf("n: %d,i: %d\n", n,i);

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fprintf(fp, "%4d\n", i + 1 + 1000);
  }

  for (i = 500; i < 1000; i ++) {
    fscanf(fp, "%d", &n);
    
    // assert(n == i + 1);
  }printf("n: %d,i: %d\n", n,i);

  fseek(fp, 0, SEEK_SET);
  for (i = 0; i < 500; i ++) {
    fscanf(fp, "%d", &n);
    
    // assert(n == i + 1 + 1000);
  }printf("n: %d,i: %d\n", n,i);

  fclose(fp);

  printf("PASS!!!\n");

  return 0;
}
