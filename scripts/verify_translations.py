#!/usr/bin/env python3
"""

OntyTask - Translation Consistency Checker

This script automatically verifies that all localization strings and keys are 100% synchronized across the entire project.
It checks the following sources:
  1. src/lang/lang.cpp                           -> Source of truth (kTable array)
  2. src/lang/lang.h                             -> C++ string IDs (enum LStrId)
  3. wiki/.translation.json                      -> Standalone JSON template
  4. .github/ISSUE_TEMPLATE/translations.yml     -> GitHub Issue template
  5. wiki/Translation-Table.md                   -> Wiki reference dictionary
  6. src/**/*.cpp & src/**/*.h                   -> Active codebase usage (detects unused/dead keys)

"""

import glob
import json
import os
import re
import sys


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)

    lang_cpp_path = os.path.join("src", "lang", "lang.cpp")
    with open(lang_cpp_path, "r", encoding="utf-8") as f:
        ktable_m = re.search(
            r"static const LEntry kTable\[L_COUNT\] = \{(.*?)\};", f.read(), re.DOTALL
        )
        if not ktable_m:
            print("[ERROR] Could not parse kTable in lang.cpp")
            return 1

        raw_entries = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*((?:L"(?:\\.|[^"\\])*"\s*)+),\s*((?:L"(?:\\.|[^"\\])*"\s*)+)\}',
            ktable_m.group(1),
        )
        cpp_keys = []
        cpp_map = {}
        for key, en_raw, ru_raw in raw_entries:
            en_str = "".join(re.findall(r'L"((?:\\.|[^"\\])*)"', en_raw))
            ru_str = "".join(re.findall(r'L"((?:\\.|[^"\\])*)"', ru_raw))
            cpp_keys.append(key)
            cpp_map[key] = en_str

    total_keys = len(cpp_keys)

    lang_h_path = os.path.join("src", "lang", "lang.h")
    with open(lang_h_path, "r", encoding="utf-8") as f:
        enum_m = re.search(r"enum LStrId \{(.*?)\};", f.read(), re.DOTALL)
        if not enum_m:
            print("[ERROR] Could not parse enum LStrId in lang.h")
            return 1
        enum_keys = [
            k.strip()
            for k in re.findall(r"\bL_[A-Z0-9_]+\b", enum_m.group(1))
            if k.strip() != "L_COUNT"
        ]

    wiki_json_path = os.path.join("wiki", ".translation.json")
    if not os.path.exists(wiki_json_path):
        wiki_json_path = os.path.join("wiki", "translation.json")
    if not os.path.exists(wiki_json_path):
        wiki_json_path = os.path.join("wiki", ".translation.json")
        template = {
            "lang": "en",
            "name": "English",
            "author": "@username",
            "strings": cpp_map,
        }
        with open(wiki_json_path, "w", encoding="utf-8") as f:
            json.dump(template, f, ensure_ascii=False, indent=2)
            f.write("\n")

    with open(wiki_json_path, "r", encoding="utf-8") as f:
        wiki_json_raw = json.load(f)
        json_dict = wiki_json_raw.get("strings", wiki_json_raw)
        json_keys = list(json_dict.keys())

    yml_path = os.path.join(".github", "ISSUE_TEMPLATE", "translations.yml")
    with open(yml_path, "r", encoding="utf-8") as f:
        json_m = re.search(
            r"value:\s*\|\s*(\{.*?\})\s*validations:", f.read(), re.DOTALL
        )
        if not json_m:
            print("[ERROR] Could not find JSON template in translations.yml")
            return 1
        raw_json = json.loads(json_m.group(1))
        yml_dict = raw_json.get("strings", raw_json)
        yml_keys = list(yml_dict.keys())

    wiki_md_path = os.path.join("wiki", "Translation-Table.md")
    with open(wiki_md_path, "r", encoding="utf-8") as f:
        wiki_lines = [
            line.split("|")[1].strip().strip("`")
            for line in f
            if line.startswith("| `")
        ]

    cpp_files = [
        f
        for f in glob.glob("src/**/*.cpp", recursive=True)
        + glob.glob("src/**/*.h", recursive=True)
        if not (f.endswith("lang.h") or f.endswith("lang.cpp"))
    ]
    all_code = "".join(
        open(f, "r", encoding="utf-8", errors="ignore").read() for f in cpp_files
    )
    unused_keys = [k for k in enum_keys if not re.search(r"\b" + k + r"\b", all_code)]

    print("Translation Dynamic Synchronization Summary:")
    print(f"  Source of truth (lang.cpp):            {total_keys} keys")
    print(f"  enum LStrId (lang.h):                  {len(enum_keys)} keys")
    print(f"  Standalone Template (translation.json):{len(json_keys)} keys")
    print(f"  Issue Template (translations.yml):     {len(yml_keys)} keys")
    print(f"  Wiki Reference (Translation-Table.md): {len(wiki_lines)} keys")
    print("end!")

    errors = []

    if len(enum_keys) != total_keys:
        errors.append(
            f"Count mismatch: lang.h enum ({len(enum_keys)}) != lang.cpp kTable ({total_keys})"
        )

    diff_json_missing = [k for k in cpp_keys if k not in json_dict]
    diff_json_extra = [k for k in json_dict if k not in cpp_map]
    if diff_json_missing:
        errors.append(f"Keys missing from wiki/translation.json: {diff_json_missing}")
    if diff_json_extra:
        errors.append(f"Extra keys in wiki/translation.json: {diff_json_extra}")

    diff_yml_missing = [k for k in cpp_keys if k not in yml_keys]
    diff_yml_extra = [k for k in yml_keys if k not in cpp_keys]
    if diff_yml_missing:
        errors.append(f"Keys missing from translations.yml: {diff_yml_missing}")
    if diff_yml_extra:
        errors.append(
            f"Extra keys in translations.yml not in lang.cpp: {diff_yml_extra}"
        )

    diff_wiki_missing = [k for k in cpp_keys if k not in wiki_lines]
    diff_wiki_extra = [k for k in wiki_lines if k not in cpp_keys]
    if diff_wiki_missing:
        errors.append(
            f"Keys missing from wiki/Translation-Table.md: {diff_wiki_missing}"
        )
    if diff_wiki_extra:
        errors.append(
            f"Extra/duplicate keys in wiki/Translation-Table.md: {diff_wiki_extra}"
        )

    if unused_keys:
        errors.append(f"Unused enum keys in codebase (dead strings): {unused_keys}")

    if errors:
        print("[FAIL] Found synchronization issues:")
        for e in errors:
            print(f"  - {e}")
        return 1

    print(
        f"[SUCCESS] All {total_keys} translation keys match across all sources and are 100% active in codebase!"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
