#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;
static int idx=3;
void naive_uload(PCB *pcb, const char *filename);
uintptr_t loader(PCB *pcb, const char *filename);
size_t fs_close(int fd);
size_t fs_open(const char *pathname, int flags, int mode);

void switch_boot_pcb() {
  current = &pcb_boot;
}

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%p' for the %dth time!", (uintptr_t)arg, j);
    j ++;
    yield();
  }
}

void context_kload(PCB* n_pcb, void (*entry)(void *), void *arg) {
  n_pcb->cp = kcontext((Area) { n_pcb->stack, n_pcb + 1 }, entry, arg);
}

size_t context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  /*
    通过new_page()分配用户栈（32KB）
    在用户栈上放置argc/argv/envp参数
    调用ucontext()创建上下文
    将用户栈顶地址设置到上下文的栈指针寄存器中
  */
  void *new_stack = new_page(8);
  uintptr_t user_sp = (uintptr_t)new_stack -1 + 8*PGSIZE; // 32KB
  int n_arg=0, n_env=0;
  for (; argv[n_arg]!=NULL; n_arg++); 
  for (; envp[n_env]!=NULL; n_env++);
  n_arg ++;
  uintptr_t arg_ptr[n_arg], env_ptr[n_env];
  user_sp -= sizeof(uintptr_t);
  *(uintptr_t *)user_sp = 0; // null pointer end
  if (*envp) {
    for (int i=n_env-1; i>=0; i--) {
      user_sp -= strlen(envp[i])+1;
      memcpy((char*)user_sp, envp[i], strlen(envp[i])+1);
      env_ptr[i] = user_sp;
    }
  }
  if (*argv) {
    for (int i=n_arg-2; i>=0; i--) {
      user_sp -= strlen(argv[i])+1;
      memcpy((char*)user_sp, argv[i], strlen(argv[i])+1);
      arg_ptr[i+1] = user_sp;
    }
  }
  user_sp -= strlen(filename)+1;
  memcpy((char*)user_sp, filename, strlen(filename)+1);
  arg_ptr[0] = user_sp;


  user_sp -= sizeof(uintptr_t); *((uintptr_t*)user_sp) = 0;
  if (n_env >= 0) {
    user_sp -= sizeof(env_ptr);
    memcpy((char*)user_sp, env_ptr, sizeof(env_ptr));
  }
  user_sp -= sizeof(uintptr_t); *((uintptr_t*)user_sp) = 0;
  if (n_arg >= 0) {
    user_sp -= sizeof(arg_ptr);
    memcpy((char*)user_sp, arg_ptr, sizeof(arg_ptr));
  }
  uintptr_t entry = loader(pcb, filename);
  if (!entry || entry==-2) {
    return -2;
  }

  pcb->cp = ucontext(NULL, (Area) { (void*)&(pcb->stack[0]), (void*)(pcb + 1) }, (void*)entry);

  user_sp -= sizeof(uintptr_t);
  *((uintptr_t*)user_sp) = n_arg;
  (pcb->cp)->GPRx = user_sp;
  return 0;
}

void init_proc() {
  // context_kload(&pcb[0], hello_fun, (void *)0);
  // context_kload(&pcb[1], hello_fun, (void *)1);
  context_uload(&pcb[0], "/bin/hello", (char*[]){NULL}, (char*[]){ NULL});
  switch_boot_pcb();

  Log("Initializing processes...");
  
  // load program here
  // naive_uload(NULL, "/bin/dummy");
  // naive_uload(NULL, "/bin/hello");
  // naive_uload(NULL,"/bin/file-test");
  // naive_uload(NULL,"/bin/timer-test");
  // naive_uload(NULL, "/bin/nslider");
  // naive_uload(NULL, "/bin/menu");
  // naive_uload(NULL,"/bin/pal");
  // naive_uload(NULL,"/bin/nterm");
}

Context* schedule(Context *prev) {
  current->cp = prev;
  while(true){
    idx = (idx+1)%4;
    if (pcb[idx].cp != 0) {
      current = &pcb[idx];
      break;
    }
  }
  return current->cp;
}
