import argparse, json, subprocess
from pathlib import Path
import yaml

parser = argparse.ArgumentParser()
parser.add_argument("--manifests-path", required=True)
parser.add_argument("--tag", required=True)
args = parser.parse_args()

version = args.tag.lstrip("v")
path = Path(args.manifests_path) / version

with open("release.json", "r", encoding="utf-8") as f:
    release = json.load(f)

notes = ""
for line in release["notes"]:
    if line.startswith("(") and line.endswith(")"):
        notes = line.strip("()")
        break

for yml in path.glob("*.yaml"):
    with open(yml, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)

    data["PackageVersion"] = version

    if data.get("ManifestType") == "installer":
        for inst in data["Installers"]:
            arch = inst["Architecture"]
            inst["InstallerUrl"] = (
                f"https://github.com/Artyomka628/DelAfterReboot/releases/download/v{version}/"
                f"DelAfterReboot-{arch}.msi"
            )

    if data.get("ManifestType") == "defaultLocale":
        data["ReleaseNotes"] = notes
        data["ReleaseNotesUrl"] = (
            f"https://github.com/Artyomka628/DelAfterReboot/releases/tag/v{version}"
        )

    with open(yml, "w", encoding="utf-8") as f:
        yaml.safe_dump(data, f, sort_keys=False)

print("Updated versions and URLs")
