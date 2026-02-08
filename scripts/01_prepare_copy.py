import argparse
import shutil
from pathlib import Path
from packaging.version import Version

parser = argparse.ArgumentParser()
parser.add_argument("--manifests-path", required=True)
args = parser.parse_args()

base = Path(args.manifests_path)
versions = [p for p in base.iterdir() if p.is_dir()]

def parse(v):
    return Version(v.name)

latest = max(versions, key=parse)

import subprocess
tag = subprocess.check_output(["git", "describe", "--tags", "--abbrev=0"], text=True).strip()
new_version = tag.lstrip("v")

dst = base / new_version
if dst.exists():
    raise RuntimeError("Target version already exists")

shutil.copytree(latest, dst)
print(f"Copied {latest.name} -> {new_version}")
