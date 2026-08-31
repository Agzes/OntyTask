# Hotkeys & Controls

OntyTask lets you trigger recordings and macros from anywhere across desktop applications and full-screen games.

---

## 1. Default Shortcuts

| Action            |                            Default Key                             | What it does                                 |
| :---------------- | :----------------------------------------------------------------: | :------------------------------------------- |
| **Record / Stop** | <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>Alt</kbd> + <kbd>R</kbd> | Starts or stops macro recording              |
| **Play / Stop**   | <kbd>Ctrl</kbd> + <kbd>Shift</kbd> + <kbd>Alt</kbd> + <kbd>P</kbd> | Starts or stops macro playback               |
| **Panic Stop**    |             <kbd>Pause</kbd> / <kbd>Scroll Lock</kbd>              | Immediately interrupts playback              |
| **Cancel Dialog** |                           <kbd>Esc</kbd>                           | Closes open menus or cancels shortcut prompt |
| **Confirm**       |                          <kbd>Enter</kbd>                          | Confirms custom speed or loop count dialogs  |

---

## 2. Setting Custom Shortcuts

You can rebind Record and Play to any single key or combination:

1. Open settings: click **Menu** on the toolbar (or right-click the tray icon).
2. Go to **Hotkeys -> Recording** or **Hotkeys -> Playback**.
3. A prompt appears: `Press a new hotkey (Esc to cancel)`.
4. Press your desired key or combination (e.g. <kbd>F8</kbd>, <kbd>Alt+F8</kbd>, <kbd>Ctrl+Alt+Q</kbd>).
5. The shortcut is updated immediately and saved to `OntyTask.ini`.

---

## 3. Panic Stop Keys (Emergency Abort)

To abort a running macro or continuous loop, you can configure several quick triggers under **Menu -> Hotkeys -> Panic stop keys**:

- **Pause / Break:** The classic keyboard pause key.
- **Scroll Lock:** Convenient single-key stop.
- **Escape (Esc):** Fast cancel key. _(Optional: can be disabled if your game macros use Esc to open in-game menus)._
- **Stop on mouse move:** When enabled, simply moving your physical mouse during playback will immediately stop the macro.

### Status Bar Indicators

- During recording or playback, the status bar displays the active stop key (e.g. `PLAY 00:03 (1/5) • Stop: Pause`).
- If you move the mouse during playback with mouse-stop disabled, a quick reminder tooltip appears: `Macro playing • Press Pause to stop`.
