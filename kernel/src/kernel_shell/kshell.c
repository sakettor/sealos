#include "../utils/utils.h"
#include "../ft/flanterm.h"
#include "../ft/flanterm_backends/fb.h"
extern struct flanterm_context *ft_ctx;
extern void output(char* str);

char command_buf[200];
int cursor_pos = 0;
const char* prompt = "user@defaultpc >> ";

char keyboard[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,     
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
    '*',    0,  ' ',    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
    0,    0,    0,    0,    -1,     0,    0,    0,    0,    0,    0,    0,    0,    
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
};

void cmd_cleanup() {
    for (int i = 0; i < 50; i++) {
        command_buf[i] = 0;
    }
    cursor_pos = 0;
    output(prompt);
}

/* command execution */
void exec(const char *str) {
    output("\n");
    if (!strcmp(str, "help")) {
        output("Commands: help\n");
        cmd_cleanup();
        return;
    }
    output("Command not found.\n");
    cmd_cleanup();
    return;
}

void handle_keyboard() {
    uint8_t scancode = inb(0x60);
    if (scancode > 127) {
        outb(0x20, 0x20); // send EOI
        return; // key release, ignore
    }
    char letter = keyboard[scancode];
    if (letter == '\n') {
        exec(command_buf);
        outb(0x20, 0x20);
        return;
    }
    flanterm_write(ft_ctx, &letter, 1);
    command_buf[cursor_pos++] = letter;
    outb(0x20, 0x20); // send EOI
}
