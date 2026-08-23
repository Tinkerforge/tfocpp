#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "statham-schema>=0.15.1",
# ]
# ///

# Converts the OCPP 2.1 JSON schemas to python classes for generator21.py.
# Schemas that statham cannot convert are reported and skipped.

import os
import subprocess
import shutil

shutil.rmtree("schema21", ignore_errors=True)
os.makedirs("schema21")

SCHEMA_DIR = "../spec/schemas-2.1/json"

failed = []
converted = []

for f in sorted(os.listdir(SCHEMA_DIR)):
    if not f.endswith(".json"):
        continue
    try:
        subprocess.check_output(
            ["statham", "--input", os.path.join(SCHEMA_DIR, f), "--output", "schema21"],
            stderr=subprocess.STDOUT,
        )
        converted.append(f)
    except subprocess.CalledProcessError as e:
        failed.append((f, e.output.decode(errors="replace").strip().splitlines()[-1] if e.output else "unknown"))

with open("schema21/__init__.py", "w") as f:
    for x in converted:
        f.write("import schema21.{}\n".format(x.replace(".json", "")))

print(f"converted: {len(converted)}")
if failed:
    print(f"failed: {len(failed)}")
    for name, err in failed:
        print(f"  {name}: {err}")
