"""Format every tracked C, C++ and Python source the same way.

    py -3 tools/format.py            rewrite the files in place
    py -3 tools/format.py --check    list the files that would change; exit 1 if any

C and C++ follow .clang-format, Python follows [tool.black] in pyproject.toml.
The pinned tools: py -3 -m pip install --user clang-format==23.1.5 black==26.10.1
(this script runs without -I so it can import them from the user site).
Vendored code (examples/imgui/, src/third_party/) is never touched.
"""

import argparse
import os
import shutil
import subprocess
import sys
import sysconfig
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
C_SUFFIXES = {'.c', '.h', '.cpp', '.hpp'}


def tracked(suffixes):
    out = subprocess.run(['git', 'ls-files'], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return [
        ROOT / p
        for p in out.splitlines()
        if Path(p).suffix in suffixes and not p.startswith(('examples/imgui/', 'src/third_party/'))
    ]


def clang_format():
    found = shutil.which('clang-format')
    if found:
        return found
    scripts = Path(sysconfig.get_path('scripts', f'{os.name}_user'))
    candidate = scripts / ('clang-format.exe' if os.name == 'nt' else 'clang-format')
    if candidate.exists():
        return str(candidate)
    sys.exit('clang-format not found: py -3 -m pip install --user clang-format==23.1.5')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true', help='report files that are not formatted')
    args = parser.parse_args()

    c_files = tracked(C_SUFFIXES)
    py_files = tracked({'.py'})
    changed = []

    tool = clang_format()
    for path in c_files:
        before = path.read_bytes()
        after = subprocess.run([tool, '--style=file', str(path)], capture_output=True, check=True).stdout
        if after != before:
            changed.append(path)
            if not args.check:
                path.write_bytes(after)

    black = [sys.executable, '-m', 'black']
    if args.check and py_files:
        result = subprocess.run(
            black + ['--check'] + [str(p) for p in py_files],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if result.returncode not in (0, 1):
            sys.exit(result.stderr)
        prefix = 'would reformat '
        changed += [Path(line[len(prefix) :]) for line in result.stderr.splitlines() if line.startswith(prefix)]
    elif py_files:
        subprocess.run(black + ['--quiet'] + [str(p) for p in py_files], cwd=ROOT, check=True)

    for path in changed:
        print(path.relative_to(ROOT) if path.is_absolute() else path)
    if args.check and changed:
        sys.exit(1)


if __name__ == '__main__':
    main()
