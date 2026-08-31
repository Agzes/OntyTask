# Themes & Customization

OntyTask provides several visual themes, tray icon options, and a configurable status overlay.

---

## 1. Visual Themes

You can switch the interface theme anytime under **Menu -> Theme -> Interface theme**:

- **Dark Theme:** A sleek dark palette designed for low-light environments.
- **Light Theme:** A high-contrast light design with native light popup menus.
- **Acrylic Theme:** A modern semi-transparent blur effect using Windows DWM composition (Windows 10 and Windows 11).

---

## 2. Tray and Window Icons

Icon appearance can be adjusted under **Menu -> Theme**:

- **Tray icon:**
    - `Follow theme`: Automatically toggles between light and dark icons based on the active theme.
    - `Dark`: Displays a dark tray icon.
    - `Light`: Displays a white/light tray icon (ideal for dark Windows taskbars).
- **Auto-minimize on Rec/Play:** When enabled, the OntyTask window automatically minimizes when recording or playing a macro to stay out of view.

---

## 3. Floating Status Bar (HUD)

OntyTask features a lightweight, borderless status HUD that gives real-time visual feedback:

- **Active State:** Displays timer and loop status (e.g. `REC 00:02 • Stop: Ctrl+Shift+Alt+R` or `PLAY 00:03 (1/5) • Stop: Pause`).
- **Toast Alerts:** Shows brief notifications when files are saved, shortcuts conflict, or settings change.

### Configuration

Under **Menu -> Status bar**:

- **Show status bar:** Toggles the HUD on or off.
- **Top / Bottom of screen:** Sets the default screen position.
- **Follow cursor monitor:** Automatically moves the status bar to whichever monitor currently contains your mouse cursor.

### Cursor Evasion

To ensure the status bar never covers clickable buttons or in-game elements:

- When your mouse cursor moves close to the status bar, it smoothly jumps to the opposite edge of the screen.
- Once the cursor leaves the area, it returns to its configured position.
