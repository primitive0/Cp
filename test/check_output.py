#!/usr/bin/env python3
import sys

SUBSTITUTIONS = {
    "r": b"\r",
    "n": b"\n",
    "t": b"\t",
    "0": b"\0",
    "%": b"%",
}


def read_file(path: str) -> str:
    with open(path, "r", encoding="utf-8") as file:
        return file.read()


def text2binary(content: str) -> bytes:
    result = bytearray()
    i = 0
    while i < len(content):
        if content.startswith("@EOF@", i):
            break

        ch = content[i]
        if ch != "%":
            result.extend(ch.encode("utf-8"))
            i += 1
            continue

        i += 1
        if i >= len(content):
            raise ValueError("dangling '%' at end of input")
        escape = content[i]
        if escape not in SUBSTITUTIONS:
            raise ValueError(f"unknown escape sequence: %{escape}")
        result.extend(SUBSTITUTIONS[escape])
        i += 1
    return bytes(result)


if __name__ == "__main__":
    actual = sys.stdin.buffer.read()
    expected = text2binary(read_file(sys.argv[1]))
    sys.exit(0 if actual == expected else 1)
