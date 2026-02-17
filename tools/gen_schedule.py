#!/usr/bin/env python3
# tools/gen_schedule.py
#
# Generates: generated/schedule_config.c
# Expects:  tools/schedule.json
#
# Output defines:
#   TimelineTaskConfig_t my_schedule[]
#   const uint32_t my_schedule_count
#   const uint32_t g_major_frame_ms, g_minor_frame_ms, g_subframe_count
#
# NOTE:
#   schedule_config.c must see prototypes for the task functions referenced
#   in JSON. Create schedule_tasks.h (in project root or config/) and ensure
#   it declares all task functions used by the schedule.
#
# Minimal JSON schema:
# {
#   "major_frame_ms": 100,
#   "minor_frame_ms": 10,
#   "tasks": [
#     {
#       "taskId": 0,
#       "task_name": "Producer",
#       "function": "vTaskProducer",
#       "type": "HARD_RT",
#       "start_ms": 2,
#       "end_ms": 5,
#       "subframe_id": 0,
#       "stack_words": 256
#     }
#   ]
# }

import json
import os
import sys
from typing import Any, Dict, List, Set

REQUIRED_TOP = ["major_frame_ms", "minor_frame_ms", "tasks"]

REQUIRED_TASK = [
    "taskId",
    "task_name",
    "function",
    "type",
    "start_ms",
    "end_ms",
    "subframe_id",
    "stack_words",
]

VALID_TYPES = {"HARD_RT", "SOFT_RT"}


def die(msg: str) -> None:
    print(f"[gen_schedule] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def load_json(path: str) -> Dict[str, Any]:
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        die(f"JSON not found: {path}")
    except json.JSONDecodeError as e:
        die(f"Invalid JSON: {e}")


def _is_int(x: Any) -> bool:
    # bool is a subclass of int, exclude it
    return isinstance(x, int) and not isinstance(x, bool)


def validate(cfg: Dict[str, Any]) -> None:
    for k in REQUIRED_TOP:
        if k not in cfg:
            die(f"Missing top-level key: {k}")

    major = cfg["major_frame_ms"]
    minor = cfg["minor_frame_ms"]

    if not _is_int(major) or major <= 0:
        die("major_frame_ms must be a positive int")
    if not _is_int(minor) or minor <= 0:
        die("minor_frame_ms must be a positive int")
    if major % minor != 0:
        die("major_frame_ms must be divisible by minor_frame_ms")

    tasks = cfg["tasks"]
    if not isinstance(tasks, list) or len(tasks) == 0:
        die("tasks must be a non-empty list")

    subframe_count = major // minor

    seen_ids: Set[int] = set()
    for i, t in enumerate(tasks):
        if not isinstance(t, dict):
            die(f"tasks[{i}] must be an object")

        for k in REQUIRED_TASK:
            if k not in t:
                die(f"tasks[{i}] missing key: {k}")

        # taskId
        if not _is_int(t["taskId"]):
            die(f"tasks[{i}].taskId must be an int")
        tid = int(t["taskId"])
        if tid < 0 or tid > 255:
            die(f"tasks[{i}] taskId must fit uint8_t (0..255), got {tid}")
        if tid in seen_ids:
            die(f"Duplicate taskId: {tid}")
        seen_ids.add(tid)

        # type
        ttype = t["type"]
        if ttype not in VALID_TYPES:
            die(f"tasks[{i}] type must be one of {sorted(VALID_TYPES)} (got {ttype})")

        # times
        if not _is_int(t["start_ms"]) or not _is_int(t["end_ms"]):
            die(f"tasks[{i}] start_ms/end_ms must be ints")
        start = int(t["start_ms"])
        end = int(t["end_ms"])
        if start < 0 or end < 0:
            die(f"tasks[{i}] start_ms/end_ms must be >= 0")

        # For HARD_RT your xValidateSchedule requires end > start
        if ttype == "HARD_RT" and end <= start:
            die(f"tasks[{i}] HARD_RT requires end_ms > start_ms (got {start}..{end})")

        # sanity: within major frame (HRT only)
        if ttype == "HARD_RT" and (start > major or end > major):
            die(f"tasks[{i}] start_ms/end_ms exceed major_frame_ms={major}")

        # subframe_id (note: your xPreprocessSchedule overwrites it for HRT anyway)
        if not _is_int(t["subframe_id"]):
            die(f"tasks[{i}].subframe_id must be an int")
        sf = int(t["subframe_id"])
        if sf < 0 or sf >= subframe_count:
            die(f"tasks[{i}] subframe_id out of range [0..{subframe_count - 1}] (got {sf})")

        # stack_words -> uint16_t
        if not _is_int(t["stack_words"]):
            die(f"tasks[{i}].stack_words must be an int")
        stack = int(t["stack_words"])
        if stack <= 0 or stack > 65535:
            die(f"tasks[{i}] stack_words must fit uint16_t (1..65535), got {stack}")

        # task_name/function basic checks
        if not isinstance(t["task_name"], str) or not t["task_name"]:
            die(f"tasks[{i}].task_name must be a non-empty string")
        if not isinstance(t["function"], str) or not t["function"]:
            die(f"tasks[{i}].function must be a non-empty string")


def c_escape_string(s: str) -> str:
    # Minimal escaping for C string literals
    return s.replace("\\", "\\\\").replace('"', '\\"')


def emit_c(cfg: Dict[str, Any]) -> str:
    major = int(cfg["major_frame_ms"])
    minor = int(cfg["minor_frame_ms"])
    subframe_count = major // minor
    tasks: List[Dict[str, Any]] = cfg["tasks"]

    lines: List[str] = []
    lines.append('#include "timeline_scheduler.h"')
    # NEW: prototypes for functions referenced in schedule JSON
    lines.append('#include "schedule_tasks.h"')
    lines.append("")
    lines.append("/* Auto-generated from schedule.json. DO NOT EDIT. */")
    lines.append("")
    lines.append(f"const uint32_t g_major_frame_ms = {major}u;")
    lines.append(f"const uint32_t g_minor_frame_ms = {minor}u;")
    lines.append(f"const uint32_t g_subframe_count = {subframe_count}u;")
    lines.append("")
    lines.append("TimelineTaskConfig_t my_schedule[] = {")
    for t in tasks:
        name = c_escape_string(t["task_name"])
        func = t["function"]
        ttype = t["type"]
        start = int(t["start_ms"])
        end = int(t["end_ms"])
        sf = int(t["subframe_id"])
        stack = int(t["stack_words"])
        tid = int(t["taskId"])

        lines.append(
            f'    {{"{name}", {func}, {ttype}, {start}u, {end}u, {sf}u, '
            f'(uint16_t){stack}u, (uint8_t){tid}u, NULL, TASK_NOT_STARTED, NULL}},'
        )
    lines.append("};")
    lines.append("")
    lines.append("const uint32_t my_schedule_count = (uint32_t)(sizeof(my_schedule)/sizeof(my_schedule[0]));")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    if len(sys.argv) != 3:
        die("Usage: gen_schedule.py <schedule.json> <out.c>")

    in_path = sys.argv[1]
    out_path = sys.argv[2]

    cfg = load_json(in_path)
    validate(cfg)

    out_dir = os.path.dirname(out_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    c_text = emit_c(cfg)
    with open(out_path, "w", encoding="utf-8") as f:
        f.write(c_text)

    print(f"[gen_schedule] Wrote: {out_path}")


if __name__ == "__main__":
    main()
