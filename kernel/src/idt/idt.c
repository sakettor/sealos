#include <stdint.h>

/* idt here */

extern void *interruptTable[256];
void output(char* str);

struct IDTEntry {
    uint16_t offset_low; 
    uint16_t selector;    
    uint8_t  ist;        
    uint8_t  type_attributes; 
    uint16_t offset_mid;  
    uint32_t offset_high;     
    uint32_t reserved;         
} __attribute__((packed));

struct IDTR {
    uint16_t limit;            
    uint64_t base;             
} __attribute__((packed));

struct IDTEntry idt[256]; // creating an array

struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t intNo;
    uint64_t errorCode;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

static char const* faultMessages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Detected Overflow",
    "Out Of Bounds",
    "Invalid OpCode",
    "No CoProcessor",
    "Double Fault",
    "CoProcessor Segment Overrun",
    "Bad Tss",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "CoProcessor Fault",
    "Alignment Check",
    "Machine Check",
    "Simd Floating Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Hypervisor Injection Exception",
    "Vmm Communication Exception",
    "Security Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void fill_idt_struct(int index, void *handler) { // split the handler's address into 3 parts and fill in the IDTEntry struct
    uint64_t address = handler;
    uint16_t low = address & 0xFFFF;
    uint16_t mid = address >> 16 & 0xFFFF;
    uint32_t high = address >> 32;
    idt[index].ist = 0;
    idt[index].offset_high = high;
    idt[index].offset_low = low;
    idt[index].offset_mid = mid;
    idt[index].reserved = 0;
    idt[index].selector = 0x28; // limine builtin GDT for now
    idt[index].type_attributes = 0x8E;
}

void load_idt() {
    output("Loading IDT...\n");
    struct IDTR idtr = {
        sizeof(idt) - 1,
        (uint64_t)&idt
    };
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    for (int i = 0; i < 256; i++) {
        fill_idt_struct(i, interruptTable[i]);
    }
    output("IDT Loaded.\n");
}

void handleInterruptAsm(struct interrupt_frame* frame) {
    if (frame->intNo < 32) {
        output("\n!!! KERNEL PANIC !!!\nException: ");
        output(faultMessages[frame->intNo]);
        output("\nDying...");
        hcf();
    } else {
        if (frame->intNo == 33) { // irq 1
            output("\nGot input\n");
        }
    }
}

void test_idt() {
    __asm__ volatile ("int $2");
}
