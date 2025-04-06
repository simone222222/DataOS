// Define our own types since we don't have the standard library
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;

// VGA text mode color constants
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
};

// Function to create a VGA color byte from foreground and background colors
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// Function to create a VGA character entry
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Constants for VGA text mode
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

// Global variables for terminal state
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;

// Initialize the terminal
void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*) 0xB8000;

    // Clear the screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

// Set the terminal color
void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

// Put a character at a specific position
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

// Put a character at the current position and advance the cursor
void terminal_putchar(char c) {
    // Handle newline
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

// Write a string to the terminal
void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}

// Memory functions
void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }
    return 0;
}

// String functions
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Simple keyboard input (PS/2 keyboard)
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

// Read a byte from a port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a byte to a port
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// US keyboard layout scancode to ASCII
char keyboard_scancode_to_char(uint8_t scancode) {
    // Simple mapping for basic keys
    const char scancode_to_ascii[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
        0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };

    // Only handle press events (ignore release)
    if (scancode & 0x80) {
        return 0;
    }

    // Return ASCII character if in range
    if (scancode < sizeof(scancode_to_ascii)) {
        return scancode_to_ascii[scancode];
    }

    return 0;
}

// Read a character from the keyboard
char keyboard_read_char(void) {
    char c = 0;
    while (c == 0) {
        // Wait for keyboard input
        if ((inb(KEYBOARD_STATUS_PORT) & 1) == 0) {
            continue;
        }

        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        c = keyboard_scancode_to_char(scancode);
    }
    return c;
}

// Read a line from the keyboard
void keyboard_read_line(char* buffer, size_t buffer_size) {
    size_t i = 0;
    while (i < buffer_size - 1) {
        char c = keyboard_read_char();

        // Handle backspace
        if (c == '\b' && i > 0) {
            terminal_putchar('\b');
            terminal_putchar(' ');
            terminal_putchar('\b');
            i--;
            continue;
        }

        // Handle enter key
        if (c == '\n') {
            buffer[i] = '\0';
            terminal_putchar('\n');
            break;
        }

        // Handle printable characters
        if (c >= ' ' && c <= '~') {
            buffer[i++] = c;
            terminal_putchar(c);
        }
    }

    // Ensure null termination
    buffer[i] = '\0';
}

// Simple command execution
void execute_command(const char* command) {
    if (strcmp(command, "help") == 0) {
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help - Display this help message\n");
        terminal_writestring("  clear - Clear the screen\n");
        terminal_writestring("  about - Display information about DataOS\n");
    } else if (strcmp(command, "clear") == 0) {
        terminal_initialize();
    } else if (strcmp(command, "about") == 0) {
        terminal_writestring("DataOS - A simple operating system written in C\n");
        terminal_writestring("Version: 0.1\n");
        terminal_writestring("Created as a demonstration of basic OS concepts\n");
    } else if (strlen(command) > 0) {
        terminal_writestring("Unknown command: ");
        terminal_writestring(command);
        terminal_writestring("\nType 'help' for a list of commands\n");
    }
}

// Kernel entry point
void kernel_main(void) {
    // Initialize terminal
    terminal_initialize();

    // Welcome message
    terminal_writestring("Welcome to DataOS!\n");
    terminal_writestring("A simple operating system written in C\n");
    terminal_writestring("\n");
    terminal_writestring("System initialized successfully.\n");
    terminal_writestring("Type 'help' for available commands.\n\n");

    // Simple command shell
    char command_buffer[256];

    while (1) {
        terminal_writestring("DataOS> ");
        keyboard_read_line(command_buffer, sizeof(command_buffer));
        execute_command(command_buffer);
    }
}