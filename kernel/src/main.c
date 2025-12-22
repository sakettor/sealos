#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "ft/flanterm.h"
#include "ft/flanterm_backends/fb.h"
#include "font/font.h"
#include "idt/idt.h"
#include "utils/utils.h"

struct flanterm_context *ft_ctx = NULL;
extern const char* prompt;
extern void write_ata(int lba, char* data);
extern char *read_ata(int lba);
extern void fat32_init();
uint8_t* bmap_virt_addr = 0;
uint64_t bmap_in_bytes = 0;
extern char* command_buf;
extern char* doc_buf;

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm = {
    .id = LIMINE_HHDM_REQUEST_ID,
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

uint64_t malloc() { // memory allocation using pmm
    for (int i = 0; i < bmap_in_bytes; i++) {
        if (bmap_virt_addr[i] == 0xFF) {
            continue;
        }
        for (int o = 0; o < 8; o++) {
            int bit = (bmap_virt_addr[i] >> o) & 1;
            if (bit == 0) {
                int page = (i * 8) + o;
                bitmap_set(page);
                uint64_t virt_addr = page * 4096 + hhdm.response->offset;
                return virt_addr;
            }
        }
    }
}

void bitmap_set(int page) {
    int byte = page / 8;
    int bit = page % 8;
    bmap_virt_addr[byte] |= (1 << bit);
}

void bitmap_clear(int page) {
    int byte = page / 8;
    int bit = page % 8;
    bmap_virt_addr[byte] &= ~(1 << bit);
}

void bitmap_init() {
    uint64_t bmap_phys_addr = 0;
    uint64_t count = memmap.response->entry_count;
    uint64_t highest_address = 0;
    char str[50];
    for (uint64_t i = 0; i < count; i++) {
    
        struct limine_memmap_entry *entry = memmap.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE || entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES || entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
            uint64_t end_of_chunk = entry->base + entry->length;

            if (end_of_chunk > highest_address) {
                highest_address = end_of_chunk;
            }
        } 
    }
    int_to_str(highest_address, str);
    output("Found RAM: ");
    output(str);
    output(" bytes\n");
    bmap_in_bytes = highest_address / 4096;
    bmap_in_bytes = (bmap_in_bytes + 7) / 8;
    for (uint64_t i = 0; i < count; i++) {
    
        struct limine_memmap_entry *entry = memmap.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > bmap_in_bytes) {
            output("Found usable RAM for bitmap allocator!\n");
            bmap_phys_addr = entry->base;
            break;
        } 
    }
    bmap_virt_addr = bmap_phys_addr + hhdm.response->offset;
    memset(bmap_virt_addr, 0xFF, bmap_in_bytes);
    output("Initialized bitmap.\033[0m\n");
    for (uint64_t i = 0; i < count; i++) {
    
        struct limine_memmap_entry *entry = memmap.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            int start_page = entry->base / 4096;
            int times = entry->length / 4096;
            for (int o = 0; o < times; o++) {
                bitmap_clear(start_page + o);
            }
        } 
    }
    int bitmap_page = bmap_phys_addr / 4096;
    int bitmap_in_pages = bmap_in_bytes / 4096;
    for (int o = 0; o < bitmap_in_pages; o++) {
        bitmap_set(bitmap_page + o);
    }
}

void output(char* str) {
    int num = strlen(str);
    flanterm_write(ft_ctx, str, num);
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
    output("\033[36m");
    load_idt();
    char width_str[20];
    char height_str[20];
        
    int_to_str(framebuffer->width, width_str);
    int_to_str(framebuffer->height, height_str);

    fat32_init();
    bitmap_init();

    output("\033[36m"); 

    output("\n\n ░▒▓███████▓▒░▒▓████████▓▒░░▒▓██████▓▒░░▒▓█▓▒░        \n");
    output("░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        \n");
    output("░▒▓█▓▒░      ░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        Resolution: ");
    output(width_str); output("x"); output(height_str); output("\n");
    output(" ░▒▓██████▓▒░░▒▓██████▓▒░ ░▒▓████████▓▒░▒▓█▓▒░        \n");
    output("       ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        SealOS version: 0.2\n");
    output("       ░▒▓█▓▒░▒▓█▓▒░      ░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░        \n");
    output("░▒▓███████▓▒░░▒▓████████▓▒░▒▓█▓▒░░▒▓█▓▒░▒▓████████▓▒░ Built with: GCC ");
    output(__VERSION__); output("\n");
    output("                                                      \n");
    output(" ░▒▓██████▓▒░ ░▒▓███████▓▒░                           \n");
    output("░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░                                  Build Date: ");
    output(__DATE__); output("\n");
    output("░▒▓█▓▒░░▒▓█▓▒░▒▓█▓▒░                                  \n");
    output("░▒▓█▓▒░░▒▓█▓▒░░▒▓██████▓▒░                            \n");
    output("░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░                           \n");
    output("░▒▓█▓▒░░▒▓█▓▒░      ░▒▓█▓▒░                           \n");
    output(" ░▒▓██████▓▒░░▒▓███████▓▒░                            \n\n");
    output("\033[36m\n"); 
    command_buf = malloc(); // using the bitmap finally
    doc_buf = malloc(); // for editing
    cmd_cleanup();

    // We're done, just hang...
    hcf();
}
