#!/usr/bin/env -S uv run --script
#
# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "statham-schema>=0.15.1",
# ]
# ///

import os
import subprocess

import shutil

shutil.rmtree("schema16")
os.makedirs("schema16")

todo = os.listdir("../spec/schemas/json")
for f in todo:
    if not f.endswith(".json"):
        continue
    subprocess.check_output(["statham", "--input", os.path.join("../spec/schemas/json", f), "--output", "schema16"])

with open("schema16/__init__.py", "w") as f:
    for x in todo:
        if not x.endswith(".json"):
            continue
        f.write("import schema16.{}\n".format(x.replace(".json", "")))
