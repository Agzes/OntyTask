#!/usr/bin/env python3

"""Checks that translations/*.json, src/lang/lang.cpp, src/lang/lang.h, and wiki/Translation-Table.md all use the same translation keys."""

import glob
import json
import os
import re
import sys


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)

    with open("src/lang/lang.cpp", "r", encoding="utf-8-sig") as f:
        lang_cpp = f.read()
    with open("src/lang/lang.h", "r", encoding="utf-8-sig") as f:
        lang_h = f.read()

    keys_m = re.search(
        r"static const char \*kKeys\[L_COUNT\] = \{(.*?)\};", lang_cpp, re.S
    )
    if not keys_m:
        print("[ERROR] Could not parse kKeys in lang.cpp")
        return 1
    cpp_keys = re.findall(r'"([^"]+)"', keys_m.group(1))

    enum_m = re.search(r"enum LStrId \{(.*?)\};", lang_h, re.S)
    if not enum_m:
        print("[ERROR] Could not parse enum LStrId in lang.h")
        return 1
    enum_keys = [
        k.strip()
        for k in re.findall(r"\bL_[A-Z0-9_]+\b", enum_m.group(1))
        if k.strip() != "L_COUNT"
    ]

    lang_files = sorted(glob.glob("translations/*.json"))
    if not lang_files:
        print("[ERROR] No translations/*.json files found")
        return 1
    languages = {}
    for path in lang_files:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        code = data.get("code") or os.path.splitext(os.path.basename(path))[0]
        languages[code] = (path, data)

    if "en" not in languages:
        print("[ERROR] translations/en.json not found")
        return 1
    en_keys = list(languages["en"][1].get("strings", {}).keys())

    print("Translation Synchronization Summary:")
    print(f"  Source keys (translations/en.json): {len(en_keys)}")
    print(f"  Key list (lang.cpp kKeys):          {len(cpp_keys)}")
    print(f"  enum LStrId (lang.h):               {len(enum_keys)}")
    print(f"  Languages:                          {', '.join(sorted(languages))}")

    errors = []

    if cpp_keys != en_keys:
        missing = [k for k in en_keys if k not in cpp_keys]
        extra = [k for k in cpp_keys if k not in en_keys]
        errors.append(
            f"lang.cpp kKeys differ from en.json (missing: {missing}, extra: {extra})"
        )
    if len(enum_keys) != len(cpp_keys):
        errors.append(
            f"Count mismatch: lang.h enum ({len(enum_keys)}) != lang.cpp kKeys ({len(cpp_keys)})"
        )

    for code, (path, data) in sorted(languages.items()):
        strings = data.get("strings", {})
        if set(strings.keys()) != set(en_keys):
            missing = [k for k in en_keys if k not in strings]
            extra = [k for k in strings if k not in en_keys]
            errors.append(f"{path}: key mismatch (missing: {missing}, extra: {extra})")
            continue
        for key, value in strings.items():
            if not isinstance(value, str):
                errors.append(f"{path}: '{key}' must be a string")

    table_path = os.path.join("wiki", "Translation-Table.md")
    if os.path.exists(table_path):
        with open(table_path, "r", encoding="utf-8") as f:
            table_keys = [
                line.split("|")[1].strip().strip("`")
                for line in f
                if line.startswith("| `")
            ]
        if set(table_keys) != set(en_keys):
            errors.append(
                "wiki/Translation-Table.md keys differ from translations/en.json"
            )

    cpp_files = [
        f
        for f in glob.glob("src/**/*.cpp", recursive=True)
        + glob.glob("src/**/*.h", recursive=True)
        if not f.endswith("lang.h")
    ]
    all_code = "".join(
        open(f, "r", encoding="utf-8", errors="ignore").read() for f in cpp_files
    )
    unused_keys = [k for k in enum_keys if not re.search(r"\b" + k + r"\b", all_code)]
    if unused_keys:
        errors.append(f"Unused enum keys in codebase: {unused_keys}")

    if errors:
        print("[FAIL]")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"[SUCCESS] All {len(en_keys)} keys are synchronized.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
