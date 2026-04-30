
#define AHB_VGA_BASE        0x50000000
#define AHB_UART_BASE       0x51000000
#define AHB_TIMER_BASE      0x52000000
#define AHB_GPIO_BASE       0x53000000
#define AHB_7SEG_BASE       0x54000000
#define NVIC_INT_ENABLE     0xE000E100
#define NVIC_INT_PRIORITY0  0xE000E400

#define COLS    20
#define ROWS    24
#define CELL    5
#define MAX_LEN 480  // max possible snake length (20*24)

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

void clear_grid(unsigned char color) {
    for (int row = 0; row < ROWS; row++)
        for (int col = 0; col < COLS; col++)
            draw_cell(col, row, color);
}

void draw_border(unsigned char color) {
    for (int col = 0; col < COLS; col++) {
        draw_cell(col, 0,        color);  // top
        draw_cell(col, ROWS - 1, color);  // bottom
    }
    for (int row = 1; row < ROWS - 1; row++) {
        draw_cell(0,        row, color);  // left
        draw_cell(COLS - 1, row, color);  // right
    }
}

// --- Snake state ---

volatile int snake_col[MAX_LEN];
volatile int snake_row[MAX_LEN];
volatile int head_idx = 2;
volatile int tail_idx = 0;
volatile int dir_x    = 1;
volatile int dir_y    = 0;

// grid: 0=empty, 1=snake — used for self-collision detection
int grid[ROWS][COLS];

// --- ISR ---

void UART_ISR() {}

void Timer_ISR() {
    // Read buttons
    *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0000;
    int input = *(volatile unsigned int*) AHB_GPIO_BASE;
    int btnU = (input >> 8) & 1;
    int btnD = (input >> 9) & 1;
    int btnL = (input >> 10) & 1;
    int btnR = (input >> 11) & 1;

    // Update direction, no 180° reversal
    if      (btnU && dir_y != 1)  { dir_x = 0;  dir_y = -1; }
    else if (btnD && dir_y != -1) { dir_x = 0;  dir_y =  1; }
    else if (btnL && dir_x != 1)  { dir_x = -1; dir_y =  0; }
    else if (btnR && dir_x != -1) { dir_x = 1;  dir_y =  0; }

    // Compute new head position with wrap-around inside the border
    int new_col = snake_col[head_idx] + dir_x;
    int new_row = snake_row[head_idx] + dir_y;

    if (new_col < 1)        new_col = COLS - 2;
    if (new_col > COLS - 2) new_col = 1;
    if (new_row < 1)        new_row = ROWS - 2;
    if (new_row > ROWS - 2) new_row = 1;

    // Self-collision check — game over: stop the timer
    if (grid[new_row][new_col] == 1) {
        *(volatile unsigned int*)(AHB_TIMER_BASE + 0x08) = 0x00;  // disable timer
        *(volatile unsigned int*)(AHB_TIMER_BASE + 0x0C) = 1;     // clear flag
        return;
    }

    // Erase tail
    grid[snake_row[tail_idx]][snake_col[tail_idx]] = 0;
    draw_cell(snake_col[tail_idx], snake_row[tail_idx], COLOR_BLACK);
    tail_idx = (tail_idx + 1) % MAX_LEN;

    // Push new head
    head_idx = (head_idx + 1) % MAX_LEN;
    snake_col[head_idx] = new_col;
    snake_row[head_idx] = new_row;
    grid[new_row][new_col] = 1;
    draw_cell(new_col, new_row, COLOR_GREEN);

    *(volatile unsigned int*)(AHB_TIMER_BASE + 0x0C) = 1;  // clear timer interrupt flag
}

// --- Main ---

int main(void) {
    // Wait for BRAM reset walk to complete
    for (volatile int i = 0; i < 100000; i++);

    // Initialize snake: 3 segments at row 12, cols 3-4-5, moving right
    snake_col[0] = 3; snake_row[0] = 12;  // tail
    snake_col[1] = 4; snake_row[1] = 12;
    snake_col[2] = 5; snake_row[2] = 12;  // head
    head_idx = 2;
    tail_idx = 0;
    dir_x = 1;
    dir_y = 0;

    // Initialize grid
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
            grid[r][c] = 0;
    grid[12][3] = 1;
    grid[12][4] = 1;
    grid[12][5] = 1;

    clear_grid(COLOR_BLACK);
    draw_border(COLOR_WHITE);

    // Draw initial snake
    for (int i = 0; i < 3; i++)
        draw_cell(snake_col[i], snake_row[i], COLOR_GREEN);

    // Timer: 6 Hz at 50 MHz
    *(volatile unsigned int*) AHB_TIMER_BASE         = 8333333;
    *(volatile unsigned int*)(AHB_TIMER_BASE + 0x08) = 0x03;

    // NVIC: enable timer interrupt (IRQ0)
    *(volatile unsigned int*) NVIC_INT_PRIORITY0 = 0x00000000;
    *(volatile unsigned int*) NVIC_INT_ENABLE    = 0x00000001;

    while(1) {}
}
