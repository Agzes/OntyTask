# The .onty File Format

OntyTask stores recorded macros in structured JSON format with the `.onty` extension. Because files are plain UTF-8 JSON, they can be inspected, version-controlled, or edited in text editors.

---

## 1. File Structure Example

A `.onty` macro contains top-level playback settings and an array of recorded input events:

```json
{
    "onty": 1,
    "speed": 1,
    "loops": 1,
    "continuous": false,
    "events": [
        {
            "t": 0,
            "e": "kd",
            "vk": 65,
            "ext": 0
        },
        {
            "t": 120,
            "e": "ku",
            "vk": 65,
            "ext": 0
        },
        {
            "t": 350,
            "e": "md",
            "b": 0,
            "x": 1280,
            "y": 720
        },
        {
            "t": 420,
            "e": "mu",
            "b": 0,
            "x": 1280,
            "y": 720
        },
        {
            "t": 600,
            "e": "mv",
            "rel": 1,
            "p": [
                [10, 0, 16],
                [15, -2, 16],
                [8, -5, 16]
            ]
        },
        {
            "t": 850,
            "e": "wh",
            "d": 120
        }
    ]
}
```

---

## 2. Properties Reference

### Root Properties

- `onty`: Schema version (integer, currently `1`).
- `speed`: Playback speed multiplier (e.g. `1` for normal, `2` for 2x, `100` for turbo, or decimal numbers like `1.5`).
- `loops`: Number of playback repetitions (integer).
- `continuous`: Infinite loop flag (`true` or `false`).
- `events`: Array of sequential input events.

### Event Types (`e`)

| Event Type | Description         | Associated Parameters                                                              |
| :--------- | :------------------ | :--------------------------------------------------------------------------------- |
| `"kd"`     | Key Down            | `vk` (virtual-key code), `ext` (extended key flag)                                 |
| `"ku"`     | Key Up              | `vk` (virtual-key code), `ext` (extended key flag)                                 |
| `"md"`     | Mouse Button Down   | `b` (`0`=Left, `1`=Right, `2`=Middle, `3`=X1, `4`=X2), `x`, `y`                    |
| `"mu"`     | Mouse Button Up     | `b` (`0`=Left, `1`=Right, `2`=Middle, `3`=X1, `4`=X2), `x`, `y`                    |
| `"mv"`     | Mouse Motion / Turn | `rel` (`1` for relative delta, `0` for absolute), `p` array of `[dx, dy, delayMs]` |
| `"wh"`     | Mouse Wheel Scroll  | `d` (signed wheel delta, e.g. `120` or `-120`)                                     |

---

## 3. Windows File Association

You can register `.onty` files with Windows Explorer under **Menu -> Associate .onty files**:

- Allows launching and running macros by double-clicking `.onty` files in File Explorer.
- Associates the custom OntyTask icon with `.onty` files.
- Can be unassociated anytime from the same menu option.

---

## 4. Programmatic Macro Creation

You can generate `.onty` files using Python, PowerShell, or any standard JSON tools:

```python
import json

macro = {
    "onty": 1,
    "speed": 1,
    "loops": 1,
    "continuous": False,
    "events": []
}

time_ms = 0
for _ in range(5):
    # Press 'A' (VK 65)
    macro["events"].append({"t": time_ms, "e": "kd", "vk": 65, "ext": 0})
    time_ms += 50
    # Release 'A'
    macro["events"].append({"t": time_ms, "e": "ku", "vk": 65, "ext": 0})
    time_ms += 200

with open("press_a_5x.onty", "w", encoding="utf-8") as f:
    json.dump(macro, f, indent=4)

print("Macro generated: press_a_5x.onty")
```
