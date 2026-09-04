# Installation, Updates & Uninstallation

OntyTask is designed to be as flexible as possible: it runs out of the box as a standalone portable application, but also supports full system integration, Windows search indexing, automatic background updates, and package manager support (WinGet).

---

## 1. Portable vs. Installed Mode

| Feature / Mode                      |      Portable Mode      |             Installed Mode              |
| :---------------------------------- | :---------------------: | :-------------------------------------: |
| **Location**                        | Any folder / USB drive  |        `%LOCALAPPDATA%\OntyTask`        |
| **Windows Search (<kbd>Win</kbd>)** |           ❌            |                   ✅                    |
| **Start Menu Shortcut**             |           ❌            |                   ✅                    |
| **Desktop Shortcut**                |           ❌            |                   ✅                    |
| **`.onty` File Association**        |   Optional (via Menu)   |             ✅ (Automatic)              |
| **Windows "Installed Apps" List**   |           ❌            |                   ✅                    |
| **Update Method**                   | Replace `.exe` manually | Auto-sync on launch or `winget upgrade` |

---

## 2. Installation Methods

### Option A: From the In-App Menu (One Click)

1. Download `OntyTask.exe` and open it from anywhere.
2. Right-click or open the menu and click **Install OntyTask to Local AppData**.
3. OntyTask will automatically:
    - Copy itself to `%LOCALAPPDATA%\OntyTask\OntyTask.exe`.
    - Create a shortcut in your **Start Menu** (`Start Menu\Programs\OntyTask.lnk`), making it instantly searchable by pressing the Windows key <kbd>Win</kbd>.
    - Create a Desktop shortcut.
    - Register the `.onty` file association.
    - Register in Windows **Installed Apps** (`Settings -> Apps -> Installed apps`).
    - The menu button will immediately switch to **Uninstall OntyTask**.

### Option B: Via WinGet (Windows Package Manager)

> [!NOTE]
> WinGet package repository submission is currently in progress. This method will be available in an upcoming release once the Microsoft package moderation is complete.

```cmd
winget install ontytask
```

### Option C: Silent / Automated CLI

```cmd
OntyTask.exe --install
```

---

## 3. Automatic Updates (Auto-Sync)

Updating OntyTask is completely seamless:

1. **Portable to Installed Auto-Sync:**
   If OntyTask is already installed on your system, and you download a newer version of `OntyTask.exe` (for example, into your _Downloads_ folder) and run it:
    - OntyTask automatically recognizes the existing installation.
    - It **synchronizes and updates** `%LOCALAPPDATA%\OntyTask\OntyTask.exe`, shortcuts, and Windows registry metadata in the background.
    - No manual copying, moving, or re-installing is needed.

2. **Via WinGet (Coming soon):**
    ```cmd
    winget upgrade ontytask
    ```

---

## 4. Uninstallation Methods

### Option A: From the In-App Menu

1. In an installed OntyTask instance, open the menu and click **Uninstall OntyTask**.
2. A confirmation prompt will ask: _"Are you sure you want to uninstall OntyTask and remove shortcuts?"_.
3. Click **Yes**. OntyTask will cleanly remove:
    - Start Menu and Desktop shortcuts.
    - `.onty` file associations.
    - Windows Add/Remove Programs registry entries.
    - The `%LOCALAPPDATA%\OntyTask` directory and executable.

### Option B: Via Windows Settings

Go to **Settings** -> **Apps** -> **Installed apps**, find **OntyTask**, and click **Uninstall**.

### Option C: Via WinGet (Coming soon)

```cmd
winget uninstall ontytask
```

### Option D: Silent CLI Command

```cmd
OntyTask.exe --uninstall
```
