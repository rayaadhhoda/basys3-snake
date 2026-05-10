
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

// grid: 0=empty, 1=snake, 2=food
int grid[ROWS][COLS];

// Food position
volatile int food_col = 0;
volatile int food_row = 0;

// Score
volatile int score = 0;
volatile int high_score = 0;

// Speed control via switches
volatile int prev_sw_speed = -1;  // force initial update
const unsigned int speed_reload[4] = {
    8333333,   // none:                 6 Hz  (normal)
    25000000,  // sw[0] (slow):         2 Hz
    8333333,   // sw[1] (normal/base):  6 Hz
    4166666    // sw[2] (fast):         12 Hz
};
const unsigned int speed_led[4] = {
    0x00,  // none:  LEDs off
    0x01,  // sw[0]: LED[0] on
    0x02,  // sw[1]: LED[1] on
    0x04   // sw[2]: LED[2] on
};

void display_score(int s) {
    *(volatile unsigned int*)(AHB_7SEG_BASE + 0x0C)  = (s / 1000) % 10; // thousands
    *(volatile unsigned int*)(AHB_7SEG_BASE + 0x08) = (s / 100)  % 10;  // hundreds
    *(volatile unsigned int*)(AHB_7SEG_BASE + 0x04) = (s / 10)   % 10;  // tens
    *(volatile unsigned int*)(AHB_7SEG_BASE + 0x00) =  s         % 10;  // ones
}

// --- LFSR pseudo-random number generator ---

volatile unsigned int lfsr = 0xACE1;  // non-zero seed

unsigned int lfsr_next() {
    // 16-bit Galois LFSR, taps at bits 16,14,13,11
    unsigned int lsb = lfsr & 1;
    lfsr >>= 1;
    if (lsb) lfsr ^= 0xB400;
    return lfsr;
}

// Spawn food at a random empty cell (avoids snake body and border)
void spawn_food() {
    int col, row;
    do {
        col = (lfsr_next() % (COLS - 2)) + 1;  // cols 1..COLS-2
        row = (lfsr_next() % (ROWS - 2)) + 1;  // rows 1..ROWS-2
    } while (grid[row][col] != 0);

    food_col = col;
    food_row = row;
    grid[row][col] = 2;
    draw_cell(col, row, COLOR_RED);
}

// --- Console print ---

void console_putc(char c) {
    *(volatile unsigned int*) AHB_VGA_BASE = c;
}

void console_print(const char* s) {
    while (*s) console_putc(*s++);
}

void console_print_int(int n) {
    if (n >= 1000) console_putc('0' + (n / 1000) % 10);
    if (n >= 100)  console_putc('0' + (n / 100)  % 10);
    if (n >= 10)   console_putc('0' + (n / 10)   % 10);
    console_putc('0' + n % 10);
}

void game_over_screen() {
    console_print("Game Over\n");
    console_print("Score: ");
    console_print_int(score);
    console_putc('\n');
    console_print("High Score: ");
    console_print_int(high_score);
    console_putc('\n');
}

// --- ISR ---

void UART_ISR() {}

void Timer_ISR() {
    // Advance LFSR every tick so seed varies with play time
    lfsr_next();

    // Read buttons
    *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0000;
    int input = *(volatile unsigned int*) AHB_GPIO_BASE;
    int btnU = (input >> 8) & 1;
    int btnD = (input >> 9) & 1;
    int btnL = (input >> 10) & 1;
    int btnR = (input >> 11) & 1;

    // Read switches for speed selection (sw[2:0] on GPIO bits [2:0])
    // Exclusive: left = slow, center = normal, right = fast
    int speed_idx = 0;  // default (normal)
    if      ((input >> 2) & 1) speed_idx = 3;  // sw[2]: fast
    else if ((input >> 1) & 1) speed_idx = 2;  // sw[1]: normal
    else if ((input >> 0) & 1) speed_idx = 1;  // sw[0]: slow

    if (speed_idx != prev_sw_speed) {
        prev_sw_speed = speed_idx;
        // Update timer reload
        *(volatile unsigned int*) AHB_TIMER_BASE = speed_reload[speed_idx];
        // Set direction to output, then write LEDs
        *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0001;
        *(volatile unsigned int*)(AHB_GPIO_BASE + 0x00) = speed_led[speed_idx];
        // Restore direction to input for next read
        *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0000;
    }

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

    // Self-collision check — game over: stop the timer and show screen
    if (grid[new_row][new_col] == 1) {
        *(volatile unsigned int*)(AHB_TIMER_BASE + 0x08) = 0x00;
        *(volatile unsigned int*)(AHB_TIMER_BASE + 0x0C) = 1;

        // Check for new high score
        int new_high = 0;
        if (score > high_score) {
            high_score = score;
            new_high = 1;
        }

        // Flash LEDs on new high score (busy-wait, timer is stopped)
        // Switch GPIO to output for LEDs
        *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0001;
        for (int f = 0; f < (new_high ? 4 : 1); f++) {
            *(volatile unsigned int*)(AHB_GPIO_BASE + 0x00) = 0xFF;  // all on
            for (volatile int d = 0; d < 2500000; d++);              // ~200ms
            *(volatile unsigned int*)(AHB_GPIO_BASE + 0x00) = 0x00;  // all off
            for (volatile int d = 0; d < 2500000; d++);
        }
        // Restore GPIO to input
        *(volatile unsigned int*)(AHB_GPIO_BASE + 0x04) = 0x0000;

        game_over_screen();
        return;
    }

    // Check food collision
    int ate_food = (new_col == food_col && new_row == food_row);

    if (!ate_food) {
        // Normal move: erase tail
        grid[snake_row[tail_idx]][snake_col[tail_idx]] = 0;
        draw_cell(snake_col[tail_idx], snake_row[tail_idx], COLOR_BLACK);
        tail_idx = (tail_idx + 1) % MAX_LEN;
    } else {
        // Ate food: skip tail erase (grow), increment score, spawn new food
        score++;
        display_score(score);
        spawn_food();
    }

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
    score = 0;

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

    // Reset 7-seg score display to 0000
    display_score(0);

    // Spawn first food
    spawn_food();

    // Timer: 6 Hz at 50 MHz
    *(volatile unsigned int*) AHB_TIMER_BASE         = 8333333;
    *(volatile unsigned int*)(AHB_TIMER_BASE + 0x08) = 0x03;

    // NVIC: enable timer interrupt (IRQ0)
    *(volatile unsigned int*) NVIC_INT_PRIORITY0 = 0x00000000;
    *(volatile unsigned int*) NVIC_INT_ENABLE    = 0x00000001;

    while(1) {}
}
