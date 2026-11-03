#!/usr/bin/env python3

"""Reports missing translation keys and adds new English keys to the other translations/*.json files."""

import argparse
import glob
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TRANS_DIR = os.path.join(ROOT, "translations")


def load_languages():
    languages = {}
    for path in sorted(glob.glob(os.path.join(TRANS_DIR, "*.json"))):
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
        code = data.get("code") or os.path.splitext(os.path.basename(path))[0]
        languages[code] = (path, data)
    return languages


def find_language(languages, needle):
    needle = needle.lower()
    for code in languages:
        if code.lower() == needle:
            return code
    for code, (_, data) in languages.items():
        if needle == data.get("name", "").lower():
            return code
    raise SystemExit(f"Unknown language: {needle}")


def save(path, data):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
        f.write("\n")


def cmd_report(args):
    languages = load_languages()
    if "en" not in languages:
        raise SystemExit("translations/en.json not found")
    en_keys = list(languages["en"][1].get("strings", {}).keys())

    selected = (
        [find_language(languages, args.lang)] if args.lang else list(languages.keys())
    )
    for code in selected:
        path, data = languages[code]
        strings = data.get("strings", {})
        missing = [key for key in en_keys if not strings.get(key)]
        extra = [key for key in strings if key not in en_keys]
        print(f"\n{code} ({data.get('name', '')}) - {len(en_keys)} keys")
        print(f"  missing: {len(missing)} | extra: {len(extra)}")
        for key in missing:
            print(f"  missing  {key}")
        for key in extra:
            print(f"  extra    {key}")
    return 0


def cmd_new(args):
    languages = load_languages()
    if "en" not in languages:
        raise SystemExit("translations/en.json not found")
    en_keys = list(languages["en"][1].get("strings", {}).keys())
    added_total = 0
    for code, (path, data) in languages.items():
        if code == "en":
            continue
        strings = data.setdefault("strings", {})
        added = 0
        for key in en_keys:
            if key not in strings:
                strings[key] = ""
                added += 1
        if added:
            save(path, data)
            print(f"{code}: added {added} missing key(s)")
            added_total += added
    if not added_total:
        print("No missing keys.")
    return 0


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")

    parser = argparse.ArgumentParser(
        description="Manage OntyTask translation JSON files."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    report = sub.add_parser("report", help="show missing translation keys")
    report.add_argument("--lang", help="filter by language code or name")
    report.set_defaults(func=cmd_report)

    new = sub.add_parser("new", help="add missing keys from en.json")
    new.set_defaults(func=cmd_new)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
