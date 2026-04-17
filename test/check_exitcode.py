#!/usr/bin/env python3
import sys
import subprocess


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: check_exitcode <path> <expected_exit_code>", file=sys.stderr)
        return 2

    path = sys.argv[1]
    try:
        expected = int(sys.argv[2])
    except ValueError:
        print("expected_exit_code must be an integer", file=sys.stderr)
        return 2

    try:
        result = subprocess.run([path])
    except FileNotFoundError:
        print(f"executable not found: {path}", file=sys.stderr)
        return 2
    except PermissionError:
        print(f"not executable: {path}", file=sys.stderr)
        return 2

    return 0 if result.returncode == expected else 1


if __name__ == "__main__":
    sys.exit(main())
