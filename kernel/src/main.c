#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "ft/flanterm.h"
#include "ft/flanterm_backends/fb.h"
#include "font/font.h"
#include "idt/idt.h"

struct flanterm_context *ft_ctx = NULL;

// Set the base revision to 4, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}
/* utilities and stuff */
size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void output(char* str) {
    int num = strlen(str);
    flanterm_write(ft_ctx, str, num);
}


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

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.

void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // Ensure we got a framebuffer.
    if (framebuffer_request.response == NULL
     || framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // here goes the framebuffer 
    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    // defining everything
    uint32_t *framebuffer_ptr = (uint32_t *)framebuffer->address;
    size_t width  = (size_t)framebuffer->width;
    size_t height = (size_t)framebuffer->height;
    size_t pitch  = (size_t)framebuffer->pitch;
    uint8_t red_mask_size   = framebuffer->red_mask_size;
    uint8_t red_mask_shift  = framebuffer->red_mask_shift;
    uint8_t green_mask_size = framebuffer->green_mask_size;
    uint8_t green_mask_shift= framebuffer->green_mask_shift;
    uint8_t blue_mask_size  = framebuffer->blue_mask_size;
    uint8_t blue_mask_shift = framebuffer->blue_mask_shift;

    ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer_ptr, width, height, pitch,
        red_mask_size, red_mask_shift,
        green_mask_size, green_mask_shift,
        blue_mask_size, blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        terminal_font, 8, 16, 1,
        0, 0,
        0, FLANTERM_FB_ROTATE_0
    );
    load_idt();
    test_idt();
    output("handler didnt work");

    // We're done, just hang...
    hcf();
}
