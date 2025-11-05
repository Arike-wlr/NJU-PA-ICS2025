#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>

static mtrace_state_t mtrace_state = {
    .enabled = false,
    .start_addr = 0,
    .end_addr = 0xffffffff,
    .read_count = 0,
    .write_count = 0
};

void init_mtrace() {
    mtrace_state.enabled = true;
    
    // 解析条件配置
    const char *cond = CONFIG_MTRACE_COND;
    if (cond[0] != '\0') {
        if (sscanf(cond, "%x-%x", &mtrace_state.start_addr, &mtrace_state.end_addr) != 2) {
            printf("Warning: Invalid mtrace condition format, tracing all accesses\n");
            mtrace_state.start_addr = 0;
            mtrace_state.end_addr = 0xffffffff;
        }
    }
    
    printf("MTRACE: enabled, condition: [0x%08x, 0x%08x]\n", 
           mtrace_state.start_addr, mtrace_state.end_addr);
}

void enable_mtrace(bool enabled) {
    mtrace_state.enabled = enabled;
    printf("MTRACE: %s\n", enabled ? "enabled" : "disabled");
}

void set_mtrace_range(uint32_t start, uint32_t end) {
    mtrace_state.start_addr = start;
    mtrace_state.end_addr = end;
    printf("MTRACE: range set to [0x%08x, 0x%08x]\n", start, end);
}

static bool should_trace(uint32_t addr, int len) {
    if (!mtrace_state.enabled) return false;
    
    // 检查地址范围是否在条件内
    return (addr >= mtrace_state.start_addr && addr + len - 1 <= mtrace_state.end_addr);
}

void mtrace_read(uint32_t addr, int len, uint32_t data) {
    if (!should_trace(addr, len)) return;
    
    mtrace_state.read_count++;
    
    const char *size_str;
    switch (len) {
        case 1: size_str = "byte"; break;
        case 2: size_str = "half"; break;
        case 4: size_str = "word"; break;
        default: size_str = "unknown"; break;
    }
    
    printf(MTRACE_FMT("READ") " addr: 0x%08x, size: %s, data: 0x%08x\n", addr, size_str, data);
}

void mtrace_write(uint32_t addr, int len, uint32_t data) {
    if (!should_trace(addr, len)) return;
    
    mtrace_state.write_count++;
    
    const char *size_str;
    switch (len) {
        case 1: size_str = "byte"; break;
        case 2: size_str = "half"; break;
        case 4: size_str = "word"; break;
        default: size_str = "unknown"; break;
    }
    
    printf(MTRACE_FMT("WRITE") " addr: 0x%08x, size: %s, data: 0x%08x\n", addr, size_str, data);
}

mtrace_state_t get_mtrace_state() {
    return mtrace_state;
}