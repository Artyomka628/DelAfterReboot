import argparse, hashlib, urllib.request
from pathlib import Path
import yaml

parser = argparse.ArgumentParser()
parser.add_argument("--manifests-path", required=True)
parser.add_argument("--tag", required=True)
args = parser.parse_args()

version = args.tag.lstrip("v")
path = Path(args.manifests_path) / version

def sha256(url):
    h = hashlib.sha256()
    with urllib.request.urlopen(url) as r:
        while True:
            b = r.read(8192)
            if not b:
                break
            h.update(b)
    return h.hexdigest().upper()

for yml in path.glob("*.installer.yaml"):
    with open(yml, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)

    for inst in data["Installers"]:
        inst["InstallerSha256"] = sha256(inst["InstallerUrl"])

    with open(yml, "w", encoding="utf-8") as f:
        yaml.safe_dump(data, f, sort_keys=False)

print("SHA256 updated")
