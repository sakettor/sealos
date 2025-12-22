#include <stdint.h>
#include <stddef.h>

unsigned char inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
void reverse(char* str, int length);
void int_to_str(uint64_t num, char* str);
size_t strlen(const char *str);
void hcf(void);
int strcmp(const char* s1, const char* s2);
char *strncpy(char *dst, const char *src, size_t n);
void outw(uint16_t port, uint16_t value);
void strcpy(char dest[], const char source[]);
int atoi(char *p);
char *strcat(char *dest, const char *src);
char* strchr(const char* s, int c);