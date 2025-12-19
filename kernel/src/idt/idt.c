#include <stdint.h>
#include "../utils/utils.h"

/* idt here */

extern void *interruptTable[256];
extern void handle_keyboard();
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

void io_wait(void) {
    outb(0x80, 0);
}

void remap_pic() {
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    io_wait();
}

void byte_to_hex_str(unsigned char byte, char* buffer) {
    const char* hex_chars = "0123456789ABCDEF";
    buffer[0] = hex_chars[(byte >> 4) & 0xF]; // High nibble
    buffer[1] = hex_chars[byte & 0xF];        // Low nibble
    buffer[2] = 0;                            // Null terminator
}

void load_idt() {
    outb(0x21, 0xFC); 
    outb(0xA1, 0xFF);
    output("Loading IDT...\n");
    remap_pic();
    uint8_t mask = inb(0x21);
    outb(0x21, mask & ~(1 << 1)); // unmask irq1
    struct IDTR idtr = {
        sizeof(idt) - 1,
        (uint64_t)&idt
    };
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    for (int i = 0; i < 256; i++) {
        fill_idt_struct(i, interruptTable[i]);
    }
    __asm__ volatile ("sti");
    outb(0x21, 0xFD); 
    output("IDT Loaded.\n");
}

void handleInterruptAsm(struct interrupt_frame* frame) {
    if (frame->intNo < 32) {
        output("\n!!! KERNEL PANIC !!!\nException: ");
        output(faultMessages[frame->intNo]);
        output("\nDying..."); // TODO: add more debug info, wtf is this
        hcf();
    } else {
        if (frame->intNo == 33) { // irq 1
            handle_keyboard();
        }
    }
}

void test_idt() {
    __asm__ volatile ("int $2");
}
