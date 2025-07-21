# NEMU

NEMU(NJU Emulator仿真器) is a simple but complete full-system emulator designed for teaching purpose.
Currently it supports x86, mips32, riscv32 and riscv64.
To build programs run above NEMU, refer to the [AM project](https://github.com/NJU-ProjectN/abstract-machine).

The main features of NEMU include
* a small monitor with a simple debugger
  * single step单步执行指令
  * **register/memory** examination寄存器 查看
  * expression evaluation without the support of symbols
  * watch point监视点
  * differential testing with reference design (e.g. EMU)
  * snapshot快照，简要说明
  
* CPU core with support of most common used instructions
  * x86
    * real mode is not supported
    * x87 floating point instructions are not supported
  * mips32
    * CP1 floating point instructions are not supported
  * riscv32
    * only RV32IM
  * riscv64
    * only RV64IM
  
* memory

* paging
  * TLB is optional (but necessary for mips32)
  * protection is not supported
  
* interrupt and exception
  * protection is not supported
  
* 5 devices
  * serial, timer, keyboard, VGA, audio
  * most of them are simplified and unprogrammable
  
* 2 types of I/O
  * port-mapped I/O and memory-mapped I/O
  
  ---

1. **命名惯例**：

   - `src/`：现代项目标准命名
   - 旧项目可能用 `source/` 或 `lib/`
   - NEMU 严格遵循 Linux 内核代码组织风格

2. **与其它目录的关系**：

   |    目录    |     用途     |
   | :--------: | :----------: |
   | `include/` | 头文件（.h） |
   |  `build/`  | 编译生成文件 |
   |  `tests/`  |   测试代码   |
   | `scripts/` |   构建脚本   |

------

### 📁 **核心目录解析**

#### 1. **`configs/`**

- **作用**：存储**构建配置模板**

- 内容示例：

  ```bash
  configs/
  ├── x86-64.config    # x86架构的默认配置
  ├── riscv32.config   # RISC-V32配置
  └── mips32.config    # MIPS架构配置
  ```

- **用途**：
  运行 `make menuconfig` 时，会根据这些模板生成 `.config` 文件，决定编译哪些功能模块。

#### 2. **`resource/`**

- **作用**：存放**静态资源文件**
- 典型内容：
  - 测试用例二进制文件 (`*.bin`)
  - 预置的磁盘镜像 (`disk.img`)
  - 字体/图标等图形资源（如 VGA 显示用的字符集）

#### 3. **`tools/`**

- **作用**：**构建工具和辅助脚本**
- 关键工具：
  - `qemu-diff`：与 QEMU 进行差分测试的工具
  - `memory-logger`：内存访问分析工具
  - `isa-simulator`：指令集验证工具

### 🛠️ **操作流程**

1. **初始化环境**：

   ```bash
   cd ics2024
   bash init.sh nemu
   cd nemu
   ```

2. **配置架构**（可选x86/mips/riscv）：

   ```bash
   make menuconfig  # 选择ISA架构
   ```

3. **编译运行**：

   ```bash
   make
   make run  # 启动调试界面
   ```

4. **常用调试命令**：

   ```bash
   (nemu) help      # 查看命令列表
   (nemu) si 10     # 单步执行10条指令
   (nemu) info r    # 查看寄存器
   (nemu) x/4 0x100 # 查看内存0x100处的4个字
   ```

### 🔧 你需要做的工作

#### ✅ 核心任务

|           文件位置           |  需要实现的内容   |  作业重点  |
| :--------------------------: | :---------------: | :--------: |
| `src/isa/[架构]/exec/exec.c` | 所有CPU指令的实现 |  主要任务  |
|  `src/isa/[架构]/decode.c`   |   指令解码逻辑    | 解析机器码 |
|     `src/cpu/difftest.c`     |   状态比对验证    |  调试验证  |
|    `src/monitor/sdb/*.c`     |   调试命令实现    |  调试工具  |
