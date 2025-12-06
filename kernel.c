/* kernel.c - Daniel Mobile OS minimal kernel
 * Compile with arm-none-eabi-gcc (arm 32-bit) cross toolchain.
 *
 * Functionality:
 *  - uart_init / uart_putc / uart_puts
 *  - simple framebuffer in memory (fixed resolution)
 *  - draw primitives: put_pixel, fill_rect
 *  - draw_text using an 8x8 bitmap font
 *  - kernel_main draws a launcher-like screen
 *
 * Notes:
 *  - This kernel is intentionally small and pedagogical.
 *  - For real hardware you need proper device probing and drivers.
 */

#include <stdint.h>
#include <stdarg.h>

/* --- UART (PL011 compatible on versatilepb) --- */
/* Versatile PB UART0 base (PL011) - QEMU maps "serial" to these registers in some configs.
   We use a common simple memory-mapped UART layout for teaching; on versatilepb you may
   need to adapt base addresses depending on qemu arguments. */
#define UART0_BASE 0x101f1000
#define UART0_DR   (*(volatile uint32_t*)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile uint32_t*)(UART0_BASE + 0x18))

static void uart_putc(char c) {
    /* Wait until TX FIFO not full */
    while (UART0_FR & (1 << 5)) {}
    UART0_DR = (uint32_t)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Simple printf - very small */
static void uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) uart_puts(buf);
}

/* Minimal vsnprintf implementation wrapper - use libc stub if available.
   For simplicity we declare the prototype; when building with newlib it'll link. */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/* --- Framebuffer --- */
/* We'll create a framebuffer at a fixed address in RAM for QEMU:
   Put it after our kernel image address space. We'll use 320x240 RGB565 for small memory. */
#define FB_WIDTH  320
#define FB_HEIGHT 240
#define FB_PIXELS (FB_WIDTH * FB_HEIGHT)
#define FB_ADDR   0x00020000  /* address in RAM we assume is safe in qemu versatilepb */
volatile uint16_t *framebuffer = (volatile uint16_t*)FB_ADDR;

/* Color helper (RGB565) */
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void fb_fill(uint16_t color) {
    for (int i = 0; i < FB_PIXELS; ++i) framebuffer[i] = color;
}

void fb_put_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    framebuffer[y * FB_WIDTH + x] = color;
}

void fb_fill_rect(int x, int y, int w, int h, uint16_t color) {
    int x2 = x + w;
    int y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > FB_WIDTH) x2 = FB_WIDTH;
    if (y2 > FB_HEIGHT) y2 = FB_HEIGHT;
    for (int yy = y; yy < y2; ++yy) {
        for (int xx = x; xx < x2; ++xx) {
            framebuffer[yy * FB_WIDTH + xx] = color;
        }
    }
}

/* 8x8 bitmap font (basic ASCII 32..127). For brevity we provide a tiny font for digits and letters used.
   Here we'll include a minimal 8x8 font for ASCII 32..127 — for production use replace with full table. */
static const uint8_t font8x8_basic[96][8] = {
    /* 32 space */
    {0,0,0,0,0,0,0,0},
    /* '!' 33 */
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    /* '"' 34 */
    {0x6C,0x6C,0x48,0x00,0x00,0x00,0x00,0x00},
    /* '#' 35 */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00},
    /* ... fill minimal set or expand as needed ... */
};
/* For safety we provide a fallback glyph */
static const uint8_t glyph_default[8] = {0x00,0x7E,0x81,0x81,0x81,0x81,0x7E,0x00};

void draw_char(int x, int y, char c, uint16_t color) {
    if (c < 32 || c > 127) {
        for (int row = 0; row < 8; ++row) {
            uint8_t bits = glyph_default[row];
            for (int col = 0; col < 8; ++col) {
                if (bits & (1 << (7-col))) fb_put_pixel(x+col, y+row, color);
            }
        }
        return;
    }
    const uint8_t *ch = font8x8_basic + (c - 32);
    for (int row = 0; row < 8; ++row) {
        uint8_t bits = ((const uint8_t*)ch)[row];
        for (int col = 0; col < 8; ++col) {
            if (bits & (1 << (7-col))) fb_put_pixel(x+col, y+row, color);
        }
    }
}

void draw_text(int x, int y, const char *s, uint16_t color) {
    int cx = x;
    while (*s) {
        draw_char(cx, y, *s++, color);
        cx += 8;
    }
}

/* --- Simple event loop and UI --- */
void draw_launcher() {
    uint16_t bg = rgb565(10, 10, 40);
    fb_fill(bg);

    uint16_t title = rgb565(220, 220, 255);
    draw_text(10, 10, "Daniel Mobile OS (minimal)", title);

    /* Draw simple buttons */
    uint16_t btn = rgb565(80, 160, 220);
    fb_fill_rect(20, 40, 80, 40, btn);
    draw_text(30, 50, "Calc", rgb565(0,0,0));

    fb_fill_rect(120, 40, 80, 40, btn);
    draw_text(130, 50, "Music", rgb565(0,0,0));

    fb_fill_rect(220, 40, 80, 40, btn);
    draw_text(230, 50, "Clock", rgb565(0,0,0));
}

/* Kernel entry from assembly */
extern void _start(void);

void kernel_main(void) {
    uart_puts("\n\n--- Daniel Mobile OS (minimal) booting ---\n");

    /* initialize framebuffer - in this demo it's already at FB_ADDR */
    /* clear screen */
    fb_fill(rgb565(0,0,0));
    draw_launcher();

    uart_printf("Framebuffer at 0x%08x  %dx%d\n", (unsigned)FB_ADDR, FB_WIDTH, FB_HEIGHT);

    /* Main loop: idle */
    while (1) {
        /* for now, just spin. To implement input handling we would poll device registers or use an IRQ. */
        for (volatile int i=0;i<100000;i++); /* simple delay */
    }
}
