# Macro Engine & 3D Games

OntyTask features an input capture and playback engine designed for both standard desktop tasks and modern 3D games.

---

## 1. Why 3D Games Require Special Handling

In standard desktop applications, mouse movements and clicks rely on absolute `(X, Y)` screen pixel coordinates.

3D games (such as Roblox, Minecraft, or first-person titles) operate differently:

1. The game hides and locks the mouse cursor in the center of the screen.
2. The game camera rotates based on relative movement deltas (`dx, dy`) read directly from the mouse hardware.
3. Standard macro tools that only update cursor coordinates fail to rotate the camera when the cursor is locked.

---

## 2. How OntyTask Handles Input

OntyTask captures and plays back actions using two complementary systems:

- **Desktop Input Capture:** Records clicks, keystrokes, dragging, and mouse wheel scrolls with millisecond precision.
- **Raw Input Stream for 3D Camera:** Listens to raw motion deltas directly from your mouse sensor. Even when the cursor is centered or locked in a 3D game, camera turns and pans are captured cleanly.
- **High-Resolution Drag Tracking:** When dragging with mouse buttons held, movements are tracked with high temporal resolution to maintain smooth tracking paths.

---

## 3. High-Precision Timing

Standard operating system sleep functions can introduce small timing variations (10-15 ms).

OntyTask uses an adaptive hybrid timing loop:

- **Longer pauses (>20 ms):** Yields execution to the operating system to maintain near 0% CPU usage.
- **Short intervals (<2 ms):** Uses high-resolution performance counters (`QueryPerformanceCounter`) to execute clicks and key sequences at the exact millisecond recorded.

---

## 4. Speed Multipliers & Turbo Mode

You can adjust playback speed anytime under **Menu -> Speed**:

- **0.5x, 0.75x:** Slows down playback (helpful for debugging complex workflows).
- **1x (Normal):** Exact real-time speed.
- **2x, 5x, 10x:** Speeds up execution proportionately.
- **100x Turbo:** Blazing fast playback while preserving drag fidelity.
- **Max Speed (Instant):** Eliminates delay intervals for instant execution.
- **Custom:** Set any multiplier from `0.01x` to `1000x`.
