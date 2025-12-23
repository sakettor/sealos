#include "../utils/utils.h"
#include "../ft/flanterm.h"
#include "../ft/flanterm_backends/fb.h"
#define MAX_ARGS 10

// argv stores pointers to the start of each word
char* argv[MAX_ARGS]; 
// argc counts how many words we found
int argc = 0; 
extern struct flanterm_context *ft_ctx;
extern void output(char* str);
extern uint64_t malloc();
extern struct SearchResults fat_search(char *filename);
extern struct SearchResults {
    unsigned int cluster;
    int entry;
    int success;
    int attr;
};
extern unsigned char disk_buffer[512];
extern unsigned int current_clust;
extern unsigned int root_clust;

char* command_buf = 0;
char* doc_buf = 0;
int cursor_pos = 0;
const char* prompt = "\033[32mS:/";
char* folder_name = 0;
int at_root = 1;
int editor_mode = 0;
int editor_clust = 0;
char filename[10];
char echo_buf[100];
int bash_mode = 0;


unsigned char keyboard[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',   
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,     
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,   
    '*',    0,  ' ',    0,    0x80,    0,    0,    0,    0,    0,    0,    0,    0,    // 0x80 for editor recognizing the F1 button
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
    0,    0,    0,    0,    -1,     0,    0,    0,    0,    0,    0,    0,    0,    
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    
};

void parse_command(char* input) {
    argc = 0;
    int i = 0;

    while (input[i] != '\0') {
        while (input[i] == ' ') {
            input[i] = '\0'; 
            i++;
        }
        if (input[i] == '\0') {
            break;
        }
        if (argc < MAX_ARGS) {
            argv[argc] = &input[i];
            argc++;
        }
        while (input[i] != ' ' && input[i] != '\0') {
            i++;
        }
    }
}

void cmd_cleanup() {
    for (int i = 0; i < 100; i++) {
        command_buf[i] = 0;
    }
    cursor_pos = 0;
    if (editor_mode == 1) {
        return;
    }
    if (bash_mode == 1) {
        return;
    }
    output(prompt);
    if (at_root == 0) {
        output(folder_name);
    }
    output(" \033[0m");
}

void *format(char *str, char *buffer) {
    for (int i = 0; i < 11; i++) {
        buffer[i] = ' ';
    }
    for (int i = 0; i < strlen(str) && i < 11; i++) {
        buffer[i] = str[i];
    }
}

void editor() {
    output("\033[2J\033[HSEALOS BUILTIN EDITOR\n\n");
    output("Editing: ");
    output(filename);
    output("\n-------------------------------------------------\n");
    editor_mode = 1;
    cursor_pos = 0;
}

void isl_run(int cluster) {
    bash_mode = 1;
    char* script_data[4096];
    memset(disk_buffer, 0, 512);
    fat_read_cluster(cluster, 1);
    strcpy(script_data, disk_buffer);

    char *line = script_data;
    char *next_line;

    while (line != NULL && *line != '\0') {
        next_line = strchr(line, '\n'); 
        if (next_line) {
            *next_line = '\0'; 
            next_line++;
        }

        char *r = strchr(line, '\r');
        if (r) *r = '\0';

        if (*line != '\0') {
            exec(line); 
        }

        line = next_line;
    }
    bash_mode = 0;
    return;
}

/* command execution */
void exec(const char *str) {
    const char *p = str;
    while (*p == ' ') p++;

    if (*p == ';') {
        p++;
        while (*p == ' ') p++;
        if (*p == '\0') {
            output("No input after ;\n");
            return;
        }
        char loop_cmd[256]; 
        int i = 0;
        while (p[i] != '\0' && i < 255) {
            loop_cmd[i] = p[i];
            i++;
        }
        loop_cmd[i] = '\0';
        while (1) {
            exec(loop_cmd);
        }
        return;
    }
    parse_command(str);
    output("\n");
    if (argv[0] == NULL) {
        cmd_cleanup();
    }
    if (!strcmp(argv[0], "help")) {
        output("help, create, mkdir, dir, edit, cd, read, isl\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "create")) {
        if (argv[1] == NULL || argv[2] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        fat_create_file(argv[1], argv[2], "txt");
        output("\n");
        cmd_cleanup();
        return;
    
    }
    if (!strcmp(argv[0], "mkdir")) {
        if (argv[1] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        fat_create_dir(argv[1]);
        output("\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "isl")) {
        if (argv[1] == NULL) {
            output("ISL V0.1 | Interpreted Script Language for SealOS\nAvailable options: init <filename> (creates a void script), run <cluster> (runs the script located at the specified cluster)\n");
            cmd_cleanup();
            return;
        }
        if (!strcmp(argv[1], "init")) {
            if (argv[2] == NULL) {
                output("Please provide a filename.\n");
                cmd_cleanup();
                return;
            }
            fat_create_file(argv[2], "echo Hello world!", "isl");
            cmd_cleanup();
            return;
        }
        if (!strcmp(argv[1], "run")) {
            if (argv[2] == NULL) {
                output("Please provide a cluster number of the ISL file that shows up in 'dir' command.\n");
                cmd_cleanup();
                return;
            }
            int cluster = atoi(argv[2]);
            isl_run(cluster);
            cmd_cleanup();
            return;
        }
        output("ISL V0.1 | Interpreted Script Language for SealOS\nAvailable options: init <filename> (creates a void script), run <cluster> (runs the script located at the specified cluster)\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "dir")) {
        fat_ls(current_clust);
        output("\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "sleep")) {
        volatile uint64_t smth = 0;
        uint64_t wait = 300000000;
        for(uint64_t i=0; i<wait; i++) {
            smth++;
        };
        if (smth == 676767) {
            smth++;
        }
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "echo")) {
        if (argv[1] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        for (int i = 1; i < argc; i++) {
            output(argv[i]);
            output(" ");
        }
        output("\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "edit")) {
        if (argv[1] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        strcpy(filename, argv[1]);
        editor();
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "reboot")) {
        int a = 0;
        __asm__ volatile ("lidt %0" : : "m"(a));
        __asm__ volatile ("int $2");
        return;
    }
    if (!strcmp(argv[0], "cd")) {
        if (argv[1] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        if (!strcmp(argv[1], "..")) {
            current_clust = root_clust;
            for (int i = 0; i < strlen(folder_name); i++) {
                folder_name[i] = 0;
            }
            output("\n");
            at_root = 1;
            cmd_cleanup();
            return;
        }
        char foldername_buffer[100];
        format(argv[1], foldername_buffer);
        struct SearchResults res = fat_search(foldername_buffer);
        if (res.success == 0) {
            output("No such directory.\n");
            cmd_cleanup();
            return;
        }
        if (res.attr != 0x10) {
            output("Not a directory.\n");
            cmd_cleanup();
            return;
        }
        current_clust = res.cluster;
        at_root = 0;
        strcat(folder_name, argv[1]);
        folder_name[strlen(folder_name)] = '/';
        output("\n");
        cmd_cleanup();
        return;
    }
    if (!strcmp(argv[0], "read")) {
        if (argv[1] == NULL) {
            output("No input.\n");
            cmd_cleanup();
            return;
        }
        int cluster = atoi(argv[1]);
        memset(disk_buffer, 0, 512);
        fat_read_cluster(cluster, 1);
        output(disk_buffer);
        output("\n");
        cmd_cleanup();
        return;
    }
    output("\033[31mCommand not found: ");
    output(argv[0]);
    output("\033[0m\n");
    cmd_cleanup();
    return;
}

void handle_keyboard() {
    uint8_t scancode = inb(0x60);
    if (scancode > 128) {
        outb(0x20, 0x20); // send EOI
        return; // key release, ignore
    }
    unsigned char letter = keyboard[scancode];
    if (editor_mode == 1) {
        if (letter == '\b') {
            output("\b \b");
            cursor_pos--;
            command_buf[cursor_pos] = ' '; // idk how that worked lol
            outb(0x20, 0x20);
            return;
        }
        if (letter == 0x80) {
            editor_mode = 0;
            output("\n");
            fat_create_file(filename, doc_buf);
            output("\033[2J\033[H");
            cmd_cleanup(); // get out of the editor
            outb(0x20, 0x20);
            return;
        }
        flanterm_write(ft_ctx, &letter, 1);
        doc_buf[cursor_pos++] = letter;
        outb(0x20, 0x20); // send EOI
        return;
    }
    if (letter == '\n') {
        exec(command_buf);
        outb(0x20, 0x20);
        return;
    }
    if (letter == '\b') {
        output("\b \b");
        cursor_pos--;
        command_buf[cursor_pos] = ' ';
        outb(0x20, 0x20);
        return;
    }
    if (letter == 0x80) {
        editor_mode = 0;
        outb(0x20, 0x20);
        return;
    }
    flanterm_write(ft_ctx, &letter, 1);
    command_buf[cursor_pos++] = letter;
    outb(0x20, 0x20); // send EOI
}
