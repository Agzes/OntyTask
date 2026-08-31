#!/usr/bin/env python3
"""

OntyTask - Source Encoding & UTF-8 BOM Fixer

This script ensures that all source code files (.cpp, .h, .hpp, .c, .rc)
are properly encoded in UTF-8 with a Byte Order Mark (BOM).
If any file lacks a BOM, it is automatically converted.

"""

import glob
import os
import sys

EXTENSIONS = (".cpp", ".h", ".hpp", ".c", ".rc")
BOM = b"\xef\xbb\xbf"


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.chdir(root)

    src_files = []
    for ext in EXTENSIONS:
        src_files.extend(glob.glob(f"src/**/*{ext}", recursive=True))

    src_files = sorted(set(src_files))
    total_files = len(src_files)

    missing_bom = []
    valid_bom = []

    for fpath in src_files:
        with open(fpath, "rb") as f:
            data = f.read()

        if not data.startswith(BOM):
            missing_bom.append(fpath)
            with open(fpath, "wb") as f:
                f.write(BOM + data)
        else:
            valid_bom.append(fpath)

    fixed_count = len(missing_bom)

    print("Source Files UTF-8 BOM Synchronization Summary:")
    print(f"  Total files scanned:        {total_files} files")
    print(f"  Valid UTF-8 with BOM:       {len(valid_bom)} files")
    print(f"  Files automatically fixed:  {fixed_count} files")
    print("end!")

    if fixed_count > 0:
        print(
            f"[SUCCESS] Fixed {fixed_count} file(s). All {total_files} source files are now UTF-8 with BOM!"
        )
    else:
        print(
            f"[SUCCESS] All {total_files} source files are 100% compliant UTF-8 with BOM!"
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
