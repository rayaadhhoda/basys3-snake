# Snake on Basys3

Classic Snake game running on a **Basys 3 FPGA board** with a **Cortex-M0 soft-core processor**, written in C and synthesized through Vivado.

## Gameplay

- 20×24 grid rendered on VGA (128x120 pixel area), with a text panel on the left showing score and high score
- Navigate the snake with the directional buttons (up/down/left/right)
- Eat red food to grow and score points
- Hitting the walls or yourself ends the game
- The 7-segment display shows your current score

## Controls

| Control | Function |
|---------|----------|
| **BTN[U/D/L/R]** | Move snake up, down, left, right |
| **SW[0]** | Slow speed (2 Hz) |
| **SW[1]** | Normal speed (6 Hz) |
| **SW[2]** | Fast speed (12 Hz) |

Speed LEDs (LED0–LED2) indicate which speed is active.

## Board Setup

### Requirements

- **Basys 3** (Artix-7 XC7A35T)
- **VGA monitor** (or any display with VGA input)
- **USB micro-B** for programming

### Hardware (Vivado)

1. Open `cmpe242-final-project.xpr` in Vivado 2025.2 (or newer)
2. Generate the bitstream
3. Program the board with the resulting `.bit` file

### Software (Keil MDK)

1. Open `KeilTest/KeilTest.uvprojx` in Keil MDK
2. Build the project (`main.c`)
3. The resulting hex file is written to `code.hex`, which gets synthesized into the FPGA bitstream via the BRAM initializer

### One-step alternative

If you have a pre-built bitstream, just program the board and the game runs immediately — the Cortex-M0 code is embedded in the FPGA fabric.

## Pin Mapping

| Peripheral | Basys3 Pin | Notes |
|------------|------------|-------|
| VGA | Standard VGA PMOD | 8-color output via RGB332 |
| Buttons | BTNU/D/L/R | Directional input |
| Switches | SW[2:0] | Speed control |
| LEDs | LD[2:0] | Speed indicator |
| 7-seg | AN[3:0], SEG[6:0] | 4-digit score display |

## Project Structure

```
├── KeilTest/
│   ├── main.c              # Game logic (C)
│   └── KeilTest.uvprojx    # Keil MDK project
├── cmpe242-final-project.srcs/
│   └── sources_1/imports/imports/
│       ├── FPGA/            # Cortex-M0 integration, logic
│       └── FPGA_prev_labs/  # AHB peripherals (VGA, GPIO, timer, etc.)
└── cmpe242-final-project.xpr  # Vivado project
```