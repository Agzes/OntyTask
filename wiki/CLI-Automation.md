# Command-Line (CLI) & Automation

OntyTask supports command-line arguments and single-instance IPC communication for integration with scripts, shortcuts, and scheduled tasks.

---

## Syntax

```bat
OntyTask.exe [macro.onty] [options]
```

Or with explicit `--file`:

```bat
OntyTask.exe --file "C:\Macros\farm.onty" --play --loop 5 --speed 2.0
```

---

## Command-Line Options

| Flag               | Short | Description                                              | Example                                 |
| :----------------- | :---: | :------------------------------------------------------- | :-------------------------------------- |
| `[path]`           |   -   | Path to a `.onty` file to load on start                  | `OntyTask.exe "farm.onty"`              |
| `--file <path>`    | `-f`  | Specifies file path (safe for spaces)                    | `OntyTask.exe -f "C:\Macros\farm.onty"` |
| `--play`           | `-p`  | Automatically plays macro after loading                  | `OntyTask.exe "farm.onty" --play`       |
| `--rec`            | `-r`  | Automatically starts recording on startup                | `OntyTask.exe --rec`                    |
| `--loop <N>`       | `-l`  | Loop count. Use `0` for infinite loop (`∞`)              | `OntyTask.exe --play --loop 5`          |
| `--speed <N>`      | `-s`  | Speed multiplier (`0.01` to `1000.0` or `turbo`)         | `OntyTask.exe --play --speed 2.5`       |
| `--min` / `--hide` | `-m`  | Launches OntyTask minimized to system tray               | `OntyTask.exe "farm.onty" --min --play` |
| `--close-after`    | `-c`  | Automatically closes OntyTask when playback ends         | `OntyTask.exe "farm.onty" --play -c`    |
| `--install`        | `-i`  | Installs OntyTask to Local AppData & registers shortcuts | `OntyTask.exe --install`                |
| `--uninstall`      | `-u`  | Uninstalls OntyTask, removes shortcuts and registry keys | `OntyTask.exe --uninstall`              |

---

## Single-Instance Behavior

If OntyTask is already open, running a command line does not spawn extra background processes. Arguments are forwarded directly to your active OntyTask window via Windows IPC messages.

---

## Automation Examples

### 1. One-Click Desktop Shortcut

Create a standard Windows shortcut:

- **Target:** `C:\OntyTask\OntyTask.exe "C:\Macros\routine.onty" --play --close-after --min`
- **Run:** Minimized

### 2. PowerShell Script

```powershell
$Onty = "C:\OntyTask\bin\x64\OntyTask.exe"
& $Onty "C:\Macros\step1.onty" --play --close-after | Out-Null
Start-Sleep -Seconds 2
& $Onty "C:\Macros\step2.onty" --play --speed 2.0 --loop 3 --close-after | Out-Null
```

### 3. Task Scheduler

In **Task Scheduler** (`taskschd.msc`), create a task with action:

- **Program:** `C:\OntyTask\OntyTask.exe`
- **Arguments:** `"C:\Macros\daily.onty" --play --close-after --min`

### 4. Background Infinite Loop

```bat
start "" "C:\OntyTask\OntyTask.exe" "monitor.onty" --loop 0 --play --min
```

To stop playback anytime:

- Press a panic key (<kbd>Pause</kbd>, <kbd>Scroll Lock</kbd>).
- Or click the OntyTask icon in your system tray!
