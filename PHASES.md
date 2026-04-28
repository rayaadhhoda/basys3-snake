what i got from claude:

Here's a clean breakdown of the 5-module split for your Snake project, with one person already anchored on clocking/core layer:

---

## Module Split — Basys3 Snake (5 People)

### 1. 🕐 Clocking & Core Infrastructure *(already assigned)*
- Xilinx MMCM / Clocking Wizard setup
- `sys_clock` (game logic domain) vs `pixel_clock` (VGA domain)
- Clock domain crossing safety (synchronizers)
- Top-level module wiring / port map
- Reset logic

---

### 2. 🎮 Input & Game FSM
**Who:** Person 2
- Button debouncer (all 4 directional + center/pause)
- Input Controller (IC) — direction register, conflict resolution (no 180° reversal)
- Top-level **Game FSM**: `IDLE → RUNNING → PAUSED → END`
- Tick generator (8 Hz from sys_clock) — drives game logic pipeline

**Interfaces with:** Clocking (tick signal), Game Logic (direction + state signals)

---

### 3. 🐍 Game Logic Engine
**Who:** Person 3
- Snake movement: head position update per tick
- Growth logic on food collision
- Food spawner (pseudo-random position, avoid snake body)
- Collision detection: wall + self
- Score counter (+1 per food, optional bonus color fruits)
- Operates entirely on **sys_clock / 8 Hz tick**

**Interfaces with:** FSM (state in/out), BRAM (read/write snake positions), VGA renderer (score)

---

### 4. 🧠 BRAM & Memory Architecture
**Who:** Person 4
- Dual-port BRAM instantiation (Xilinx RAMB36/RAMB18)
- **Active buffer** — live snake body coordinates (head/tail pointer, circular queue of `[x,y]` pairs)
- **Display buffer** — occupancy grid (40×30 = 1200 cells, 2 bits each: empty/snake/food/wall)
- Read port for renderer, write port for game logic
- Buffer swap / sync handshake between game clock and pixel clock domains

**Interfaces with:** Game Logic (writes), VGA Renderer (reads occupancy grid)

---

### 5. 📺 VGA Controller & Pixel Renderer
**Who:** Person 5
- VGA timing controller: 640×480 @ 60Hz, hsync/vsync, active region
- Pixel address → grid cell mapping (`x >> 4`, `y >> 4` for 16×16 cells)
- Pixel Renderer: reads occupancy buffer → outputs RGB
  - Snake body color, head color, food color, background, border
- Score display (7-seg or on-screen via simple digit ROM)
- Operates on **pixel_clock**

**Interfaces with:** BRAM (read-only display buffer), Clocking (pixel_clock)

---

## Dependency / Integration Order

```
Clocking → FSM+Input → Game Logic → BRAM → VGA
              ↓                ↑
           (tick)         (occupancy)
```

Suggested integration milestone: get a static grid rendering on VGA first (Person 5 + 4 can stub BRAM), then wire in game logic, then FSM last.

---

## Quick Interface Contract Summary

| Signal | From | To |
|---|---|---|
| `tick` (8Hz) | Clocking/FSM | Game Logic |
| `game_state` | FSM | Game Logic, Renderer |
| `direction` | Input→FSM | Game Logic |
| `bram_wr_*` | Game Logic | BRAM |
| `bram_rd_*` | BRAM | VGA Renderer |
| `pixel_clk` | Clocking | VGA |
| `sys_clk` | Clocking | All game-side modules |

Want me to draft the Verilog port declarations / interface stubs for any of these modules so the team has a shared contract to build against?