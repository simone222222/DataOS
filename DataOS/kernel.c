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

// Scroll the terminal up one line
void terminal_scroll() {
    // Move each line up one position
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t current_index = y * VGA_WIDTH + x;
            size_t next_line_index = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[current_index] = terminal_buffer[next_line_index];
        }
    }

    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
}

// Put a character at the current position and advance the cursor
void terminal_putchar(char c) {
    // Handle newline
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
        return;
    }

    // Handle backspace
    if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        }
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
            terminal_row = VGA_HEIGHT - 1;
        }
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

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
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

// Simple Snake game
void play_snake_game() {
    // Clear the screen
    terminal_initialize();

    // Game variables
    #define SNAKE_MAX_LENGTH 100
    #define GAME_WIDTH (VGA_WIDTH - 2)
    #define GAME_HEIGHT (VGA_HEIGHT - 2)

    // Snake position
    size_t snake_x[SNAKE_MAX_LENGTH];
    size_t snake_y[SNAKE_MAX_LENGTH];
    size_t snake_length = 3;

    // Initial snake position
    for (size_t i = 0; i < snake_length; i++) {
        snake_x[i] = 10 - i;
        snake_y[i] = 10;
    }

    // Food position
    size_t food_x = 15;
    size_t food_y = 15;

    // Direction (0=right, 1=down, 2=left, 3=up)
    int direction = 0;

    // Game state
    int game_over = 0;
    int score = 0;

    // Draw border
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), x, 0);
        terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), x, VGA_HEIGHT - 1);
    }

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 0, y);
        terminal_putentryat('#', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 1, y);
    }

    // Game instructions
    terminal_putentryat('S', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 2, 1);
    terminal_putentryat('c', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 3, 1);
    terminal_putentryat('o', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 4, 1);
    terminal_putentryat('r', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 5, 1);
    terminal_putentryat('e', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 6, 1);
    terminal_putentryat(':', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 7, 1);
    terminal_putentryat('0', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 9, 1);

    terminal_putentryat('Q', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 10, 1);
    terminal_putentryat('u', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 9, 1);
    terminal_putentryat('i', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 8, 1);
    terminal_putentryat('t', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 7, 1);
    terminal_putentryat(':', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 6, 1);
    terminal_putentryat('E', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 4, 1);
    terminal_putentryat('S', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 3, 1);
    terminal_putentryat('C', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), VGA_WIDTH - 2, 1);

    // Game loop
    while (!game_over) {
        // Draw food
        terminal_putentryat('*', vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK), food_x, food_y);

        // Draw snake
        for (size_t i = 0; i < snake_length; i++) {
            if (i == 0) {
                terminal_putentryat('O', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), snake_x[i], snake_y[i]);
            } else {
                terminal_putentryat('o', vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK), snake_x[i], snake_y[i]);
            }
        }

        // Delay
        for (int i = 0; i < 500000; i++) {
            asm volatile("nop");
        }

        // Check for keyboard input
        if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
            uint8_t scancode = inb(KEYBOARD_DATA_PORT);

            // Handle arrow keys and ESC
            if (scancode == 0x48 && direction != 1) { // Up arrow
                direction = 3;
            } else if (scancode == 0x50 && direction != 3) { // Down arrow
                direction = 1;
            } else if (scancode == 0x4B && direction != 0) { // Left arrow
                direction = 2;
            } else if (scancode == 0x4D && direction != 2) { // Right arrow
                direction = 0;
            } else if (scancode == 0x01) { // ESC
                game_over = 1;
                break;
            }
        }

        // Clear old snake tail
        terminal_putentryat(' ', terminal_color, snake_x[snake_length - 1], snake_y[snake_length - 1]);

        // Move snake body
        for (size_t i = snake_length - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }

        // Move snake head
        if (direction == 0) snake_x[0]++;
        else if (direction == 1) snake_y[0]++;
        else if (direction == 2) snake_x[0]--;
        else if (direction == 3) snake_y[0]--;

        // Check for collisions with walls
        if (snake_x[0] <= 0 || snake_x[0] >= VGA_WIDTH - 1 ||
            snake_y[0] <= 0 || snake_y[0] >= VGA_HEIGHT - 1) {
            game_over = 1;
        }

        // Check for collisions with self
        for (size_t i = 1; i < snake_length; i++) {
            if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
                game_over = 1;
            }
        }

        // Check for food collision
        if (snake_x[0] == food_x && snake_y[0] == food_y) {
            // Increase score
            score++;

            // Update score display
            terminal_putentryat('0' + score, vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK), 9, 1);

            // Increase snake length
            if (snake_length < SNAKE_MAX_LENGTH) {
                snake_length++;
            }

            // Generate new food position
            food_x = 1 + (inb(KEYBOARD_DATA_PORT) % (GAME_WIDTH - 1));
            food_y = 2 + (inb(KEYBOARD_DATA_PORT) % (GAME_HEIGHT - 2));
        }
    }

    // Game over message
    terminal_initialize();
    terminal_writestring("Game Over!\n");
    terminal_writestring("Your score: ");
    terminal_putchar('0' + score);
    terminal_writestring("\n\nPress any key to return to the shell...");

    // Wait for keypress
    keyboard_read_char();
    terminal_initialize();
}

// Simple command execution
void execute_command(const char* command) {
    if (strcmp(command, "help") == 0) {
        terminal_writestring("Available commands:\n");
        terminal_writestring("  help - Display this help message\n");
        terminal_writestring("  clear - Clear the screen\n");
        terminal_writestring("  about - Display information about DataOS\n");
        terminal_writestring("  ver - Display version information\n");
        terminal_writestring("  echo [text] - Display text on the screen\n");
        terminal_writestring("  color [fg] [bg] - Change terminal color\n");
        terminal_writestring("  snake - Play the Snake game\n");
        terminal_writestring("  reboot - Restart the system\n");
    } else if (strcmp(command, "clear") == 0) {
        terminal_initialize();
    } else if (strcmp(command, "about") == 0) {
        terminal_writestring("DataOS - A simple operating system written in C\n");
        terminal_writestring("Version: 0.2.0\n");
        terminal_writestring("Created as a demonstration of basic OS concepts\n");
    } else if (strcmp(command, "ver") == 0) {
        terminal_writestring("DataOS Version: 0.2.0\n");
    } else if (strncmp(command, "echo ", 5) == 0) {
        // Echo command - print the text after "echo "
        terminal_writestring(command + 5);
        terminal_writestring("\n");
    } else if (strncmp(command, "color ", 6) == 0) {
        // Simple color command - expects two numbers for fg and bg
        char fg = command[6] - '0';
        char bg = command[8] - '0';

        if (fg >= 0 && fg < 16 && bg >= 0 && bg < 16) {
            terminal_color = vga_entry_color((enum vga_color)fg, (enum vga_color)bg);
            terminal_writestring("Terminal color changed.\n");
        } else {
            terminal_writestring("Invalid color values. Use numbers 0-15.\n");
        }
    } else if (strcmp(command, "snake") == 0) {
        play_snake_game();
    } else if (strcmp(command, "reboot") == 0) {
        // Reboot the computer using the keyboard controller
        terminal_writestring("Rebooting...\n");

        // Wait for the message to be displayed
        for (int i = 0; i < 100000; i++) {
            // Simple delay
            asm volatile("nop");
        }

        // Reboot using the keyboard controller
        outb(0x64, 0xFE);

        // If that didn't work, halt
        while (1) {
            asm volatile("hlt");
        }
    } else if (strlen(command) > 0) {
        terminal_writestring("Sorry, unknown command :( ");
        terminal_writestring("\nType 'help' for a list of commands\n");
    }
}

// Kernel entry point
void kernel_main(void) {
    // Initialize terminal
    terminal_initialize();

    // ascii art logo
    terminal_writestring("\n");
    terminal_writestring("      _______       _______       _______      \n");
    terminal_writestring("     |       |     |       |     |       |     \n");
    terminal_writestring("     |  ____  |     |  ____  |     |  ____  |     \n");
    terminal_writestring("     | |    | |     | |    | |     | |    | |     \n");
    terminal_writestring("     | |____| |_____| |____| |_____| |____| |     \n");
    terminal_writestring("     |_______|     |_______|     |_______|     \n");
    terminal_writestring("\n");
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
