/* hdd driver */

#include "../utils/utils.h"
#define BSY 0x80
#define DRQ 0x08
extern void output(char* str);
extern void *memcpy(void *restrict dest, const void *restrict src, size_t n);
extern unsigned char map_buffer[512];
extern unsigned char dir_buffer[512];

char msg[512] = {0}; 
unsigned char disk_buffer[512];

uint16_t *ptr = (uint16_t *)msg;

void write_ata(int lba, char* data) {
    while (inb(0x1F7) & BSY) {
        
    }
    memset(msg, 0, 512);
    memcpy(msg, data, 512);
    uint8_t part1 = (uint8_t)(lba & 0xFF); // data sending thingy
    uint8_t part2 = (uint8_t)((lba >> 8) & 0xFF);
    uint8_t part3 = (uint8_t)((lba >> 16) & 0xFF);
    uint8_t part4 = (uint8_t)((lba >> 24) & 0x0F);
    outb(0x1F3, part1);
    outb(0x1F4, part2);
    outb(0x1F5, part3);
    outb(0x1F6, 0xE0 | part4); // ooh primary
    inb(0x1F7); inb(0x1F7); inb(0x1F7); inb(0x1F7);
    outb(0x1F2, 1);
    outb(0x1F7, 0x30); // write command
    while ( !(inb(0x1F7) & DRQ) ) {
    
    }
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]); 
    }
    while (inb(0x1F7) & BSY) { }
}

void read_ata(int lba, int buf) {
    if (inb(0x1F7) == 0xFF) {
        return;
    }
    while (inb(0x1F7) & BSY) {

    }
    uint8_t part1 = (uint8_t)(lba & 0xFF); // sending the lba we want to read
    uint8_t part2 = (uint8_t)((lba >> 8) & 0xFF);
    uint8_t part3 = (uint8_t)((lba >> 16) & 0xFF);
    uint8_t part4 = (uint8_t)((lba >> 24) & 0x0F);
    outb(0x1F3, part1);
    outb(0x1F4, part2);
    outb(0x1F5, part3);
    outb(0x1F6, 0xE0 | part4);
    inb(0x1F7); inb(0x1F7); inb(0x1F7); inb(0x1F7);
    outb(0x1F2, 1);
    outb(0x1F7, 0x20);
    while ( !(inb(0x1F7) & DRQ) ) { // waiting for hdd to be free
        
    }
    for (int i = 0; i < 256; i++) {
        uint16_t package = inw(0x1F0); // getting thingy (2 bytes)
        uint8_t first = package & 0xFF;
        uint8_t second = package >> 8;
        if (buf == 1) {
            disk_buffer[(i * 2) + 1] = second;
            disk_buffer[i * 2] = first;
        }
        if (buf == 2) {
            map_buffer[(i * 2) + 1] = second;
            map_buffer[i * 2] = first;
        }
        if (buf == 3) {
            dir_buffer[(i * 2) + 1] = second;
            dir_buffer[i * 2] = first;
        }
    }
}