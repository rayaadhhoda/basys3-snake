
#define AHB_VGA_BASE        0x50000000
#define AHB_UART_BASE       0x51000000
#define AHB_TIMER_BASE      0x52000000
#define AHB_GPIO_BASE       0x53000000
#define AHB_7SEG_BASE       0x54000000
#define NVIC_INT_ENABLE     0xE000E100
#define NVIC_INT_PRIORITY0  0xE000E400

#define COLS 20
#define ROWS 24
#define CELL 5

#define COLOR_BLACK  0x00
#define COLOR_WHITE  0xFF
#define COLOR_RED    0xE0
#define COLOR_GREEN  0x1C
#define COLOR_BLUE   0x03

// --- VGA primitives ---

void draw_pixel(int lx, int ly, unsigned char color) {
    int idx = ly * 128 + lx;
    *(volatile unsigned int*)(AHB_VGA_BASE + (idx + 1) * 4) = color;
}

void draw_cell(int col, int row, unsigned char color) {
    for (int dy = 0; dy < CELL; dy++)
        for (int dx = 0; dx < CELL; dx++)
            draw_pixel(col * CELL + dx, row * CELL + dy, color);
}

// --- ISR stubs (required by cm0dsasm.s vector table) ---

void UART_ISR()  {}
void Timer_ISR() {}

// --- Main ---

int main(void) {
    // Wait for BRAM reset walk to complete before drawing
    for (volatile int i = 0; i < 100000; i++);

    // Draw a green rectangle in the center of the grid
    for (int row = 8; row < 16; row++)
        for (int col = 6; col < 14; col++)
            draw_cell(col, row, COLOR_GREEN);

    while(1) {}
}
