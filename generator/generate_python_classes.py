import os
import subprocess

import shutil

shutil.rmtree("schema")
os.makedirs("schema")

todo = os.listdir("../spec/schemas/json")
for f in todo:
    if not f.endswith(".json"):
        continue
    subprocess.check_output(["statham", "--input", os.path.join("../spec/schemas/json", f), "--output", "schema"])

with open("schema/__init__.py", "w") as f:
    for x in todo:
        if not x.endswith(".json"):
            continue
        f.write("import schema.{}\n".format(x.replace(".json", "")))
