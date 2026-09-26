#!/usr/bin/env python3
import argparse
import json
import re
import sys

PREFIX = re.compile(r"^(\S+ \[[^\]]*\] )(.*)$")


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Rewrite every patch-diff line of a probe wire-*.jsonl as the full patch it stands for; "
                    "every other line is copied unchanged.",
    )
    parser.add_argument("input", help="wire-*.jsonl written by the probe (<ISO ts> [<Phase>] <json> per line)")
    parser.add_argument("-o", "--output", help="file to write; standard output when omitted")

    return parser.parse_args()


def split_ending(line):
    content = line.rstrip("\r\n")

    return content, line[len(content):]


def pointer_tokens(pointer):
    if pointer == "":
        return []

    return [token.replace("~1", "/").replace("~0", "~") for token in pointer.split("/")[1:]]


def child(container, token):
    if isinstance(container, list):
        return container[int(token)]

    return container[token]


def apply_operation(document, operation):
    tokens = pointer_tokens(operation["path"])
    kind = operation["op"]
    if not tokens:
        return operation.get("value")

    parent = document
    for token in tokens[:-1]:
        parent = child(parent, token)

    last = tokens[-1]
    if isinstance(parent, list):
        index = len(parent) if last == "-" else int(last)
        if kind == "add":
            parent.insert(index, operation["value"])
        elif kind == "remove":
            del parent[index]
        elif kind == "replace":
            parent[index] = operation["value"]
        else:
            raise ValueError(f"unsupported op {kind}")
    elif kind in ("add", "replace"):
        parent[last] = operation["value"]
    elif kind == "remove":
        del parent[last]
    else:
        raise ValueError(f"unsupported op {kind}")

    return document


def apply_patch(document, operations):
    for operation in operations:
        document = apply_operation(document, operation)

    return document


def full_patch(message, value):
    patch = {"type": "patch"}
    if "ts" in message:
        patch["ts"] = message["ts"]
    patch["path"] = message["path"]
    patch["value"] = value

    return json.dumps(patch, ensure_ascii=False, separators=(",", ":"))


def expand_line(content, memo, line_number):
    match = PREFIX.match(content)
    if not match:
        return content

    prefix, payload = match.groups()
    try:
        message = json.loads(payload)
    except ValueError:
        return content

    if not isinstance(message, dict):
        return content

    kind = message.get("type")
    if kind == "snapshot":
        memo.clear()
    elif kind == "patch" and isinstance(message.get("path"), str) and "value" in message:
        memo[message["path"]] = message["value"]
    elif kind == "patch-diff":
        path = message.get("path")
        if path not in memo:
            print(f"line {line_number}: patch-diff for {path} with no earlier value; kept as is", file=sys.stderr)

            return content

        value = apply_patch(memo[path], message.get("ops", []))
        memo[path] = value

        return prefix + full_patch(message, value)

    return content


def expand(source, target):
    memo = {}
    for line_number, line in enumerate(source, start=1):
        content, ending = split_ending(line)
        target.write(expand_line(content, memo, line_number) + ending)


def main():
    arguments = parse_arguments()
    with open(arguments.input, encoding="utf-8", newline="") as source:
        if arguments.output:
            with open(arguments.output, "w", encoding="utf-8", newline="") as target:
                expand(source, target)
        else:
            sys.stdout.reconfigure(encoding="utf-8", newline="")
            expand(source, sys.stdout)


if __name__ == "__main__":
    main()
