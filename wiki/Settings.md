# Themes & Customization

OntyTask provides several visual themes, tray icon options, and a configurable status overlay.

---

## 1. Visual Themes

You can switch the interface theme anytime under **Interface** in the menu:

- **Dark Theme:** A sleek dark palette designed for low-light environments.
- **Light Theme:** A high-contrast light design with native light popup menus.
- **Acrylic Theme:** A modern semi-transparent blur effect using Windows DWM composition (Windows 10 and Windows 11).

To use a JSON theme, open **Interface > Import theme JSON...**. The menu shows the active theme, and only the selected theme is checked. See the [theme gallery](Themes.md) for official palettes.

---

## 2. Tray and Window Icons

Icon appearance can be adjusted under **Interface** in the menu:

- **Tray icon:**
    - `Follow theme`: Automatically toggles between light and dark icons based on the active theme.
    - `Dark`: Displays a dark tray icon.
    - `Light`: Displays a white/light tray icon (ideal for dark Windows taskbars).
- **Auto-minimize on Rec/Play:** When enabled, the OntyTask window automatically minimizes when recording or playing a macro to stay out of view.

---

## 3. Floating Status Bar (HUD)

OntyTask features a lightweight, borderless status HUD that gives real-time visual feedback:

- **Active State:** Displays timer and loop status, plus the remaining time and a progress bar for a fixed loop count (e.g. `PLAY 00:03 (1/5) ETA 00:12`).
- **Toast Alerts:** Shows brief notifications when files are saved, shortcuts conflict, or settings change.

### Configuration

Under **Status bar** in the menu:

- **Show status bar:** Toggles the HUD on or off.
- **Progress bar:** Shows overall playback progress across the full width of the HUD.
- **ETA:** Shows the estimated time left during playback.
- **Top / Bottom of screen:** Sets the default screen position.
- **Left / Center / Right:** Sets the horizontal position. Together with top or bottom this gives all four corners.
- **Follow cursor monitor:** Automatically moves the status bar to whichever monitor currently contains your mouse cursor.

Progress and ETA are shown only for a fixed number of loops. In continuous mode they are hidden.

### Cursor Evasion

To ensure the status bar never covers clickable buttons or in-game elements:

- When your mouse cursor moves close to the status bar, it smoothly jumps to the opposite edge of the screen.
- Once the cursor leaves the area, it returns to its configured position.

---

## 4. Autoclicker Mode

Use the mode button in the toolbar to switch between **Onty** (macro recorder) and **Clicker**. Clicker mode expands the window with settings:

- **Interval:** Delay between actions in milliseconds. Values up to `999999999999` are accepted. Click the value to change it.
- **Target:** Mouse or keyboard.
- **Button / Key:** Mouse button (left, right, middle, X1, X2), or the key captured from the keyboard.
- **Type:** Single, double, or hold. Hold presses the button or key for the configured hold time on each cycle.
- **Hold:** Hold duration in milliseconds. This row appears when Type is set to Hold.

The toolbar buttons become **Start** and **Stop**, and the main menu shows clicker controls instead of macro speed and loop options. The clicker uses the same hotkey as macro playback; it is available in the clicker menu under **Hotkeys**.

While the OntyTask window is focused and the mouse cursor is inside it, the clicker skips actions so it does not click its own interface. Mouse clicking resumes when the cursor leaves the window or the window loses focus; keyboard clicking resumes when OntyTask is no longer focused.
