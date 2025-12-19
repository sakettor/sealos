#include <stdint.h>
#include <stddef.h>

unsigned char inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
void reverse(char* str, int length);
void int_to_str(uint64_t num, char* str);
size_t strlen(const char *str);
void hcf(void);
int strcmp(const char* s1, const char* s2);