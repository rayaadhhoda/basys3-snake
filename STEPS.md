Step 1 — draw_pixel / draw_cell + static screen
Write the VGA primitives and draw a static colored rectangle on screen. No game logic yet.

Verify: Rectangle appears on the right side of the VGA display at the correct position and color.

Step 2 — Draw the border
Use draw_cell to draw a wall border around the full 20×24 grid.

Verify: A solid border appears on screen, inset is black/empty.

Step 3 — Timer ISR + moving square
Set up the timer at 8 Hz. On each tick, move a single square (no snake yet) across the screen.

Verify: A colored square moves smoothly at a visible speed.

Step 4 — Button direction control
Read buttons in the ISR and change the moving square's direction based on input.

Verify: Square changes direction when you press buttons, no 180° reversal needed yet.

Step 5 — Snake body (circular buffer)
Replace the single square with a proper snake — push head, pop tail each tick. Start with a fixed length of 3.

Verify: A 3-cell snake moves and the tail erases correctly with no visual artifacts.

Step 6 — Collision detection
Add wall collision and self collision → trigger game over state (just stop the game for now).

Verify: Snake stops when it hits the border or itself.

Step 7 — Food + growth
Spawn a food cell at a random empty position. On eating it, skip the tail pop (grow by 1) and spawn new food.

Verify: Food appears, snake grows when it hits it, new food spawns elsewhere.

Step 8 — Score on 7-seg
Increment a score counter on each food eaten, display it on the 7-segment display.

Verify: Score increments correctly on the display each time food is eaten.

Step 9 — Game over + restart
On game over, print "GAME OVER" to the VGA console (left side). Press BTNC (reset) to restart.

Verify: Game over message appears, reset restarts cleanly.