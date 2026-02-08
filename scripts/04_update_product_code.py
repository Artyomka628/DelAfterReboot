import argparse
import re
from pathlib import Path
from packaging.version import Version
import yaml

parser = argparse.ArgumentParser()
parser.add_argument("--manifests-path", required=True)
parser.add_argument("--wxs-path", required=True)
args = parser.parse_args()

base = Path(args.manifests_path)

# find latest version folder
versions = [p for p in base.iterdir() if p.is_dir()]
if not versions:
    raise RuntimeError("No version folders found")

latest = max(versions, key=lambda p: Version(p.name))
print(f"Using manifest version folder: {latest.name}")

installer_files = list(latest.glob("*.installer.yaml"))
if not installer_files:
    raise RuntimeError("No *.installer.yaml found")

installer_path = installer_files[0]

with open(installer_path, "r", encoding="utf-8") as f:
    data = yaml.safe_load(f)

for inst in data.get("Installers", []):
    arch = inst["Architecture"]
    wxs = Path(args.wxs_path) / f"Product-{arch}.wxs"

    if not wxs.exists():
        raise RuntimeError(f"WXS not found: {wxs}")

    text = wxs.read_text(encoding="utf-8")
    m = re.search(r'Id="([A-F0-9\-]{36})"', text, re.IGNORECASE)
    if not m:
        raise RuntimeError(f"Product Id not found in {wxs}")

    product_id = m.group(1).upper()
    inst["ProductCode"] = f"{{{product_id}}}"
    print(f"Found Product Id for {arch}: {product_id}")

with open(installer_path, "w", encoding="utf-8") as f:
    yaml.safe_dump(data, f, sort_keys=False)

print("ProductCode updated successfully")
