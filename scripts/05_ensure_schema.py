import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--manifests-path", required=True)
args = parser.parse_args()

schemas = {
    "installer": "https://aka.ms/winget-manifest.installer.1.10.0.schema.json",
    "defaultLocale": "https://aka.ms/winget-manifest.defaultLocale.1.10.0.schema.json",
    "version": "https://aka.ms/winget-manifest.version.1.10.0.schema.json",
}

processed = 0
updated = 0

for yml in Path(args.manifests_path).glob("*.yaml"):
    processed += 1
    print(f"Processing file: {yml}")

    lines = yml.read_text(encoding="utf-8").splitlines()
    mtype = None
    for l in lines:
        l_stripped = l.strip()
        if l_stripped.startswith("ManifestType:"):
            mtype = l_stripped.split(":", 1)[1].strip()
            break

    if mtype not in schemas:
        print(f"  [SKIP] Unknown ManifestType in {yml}")
        continue

    old_lines = lines.copy()
    lines = [l for l in lines if not l.strip().startswith("# yaml-language-server:")]

    header = f"# yaml-language-server: $schema={schemas[mtype]}"
    new_content = header + "\n" + "\n".join(lines) + "\n"

    if old_lines != lines or not old_lines[0].strip().startswith("# yaml-language-server:"):
        yml.write_text(new_content, encoding="utf-8")
        print(f"  [UPDATED] Added schema header for ManifestType: {mtype}")
        updated += 1
    else:
        print(f"  [UNCHANGED] Schema already present")

print(f"\nProcessed {processed} files, updated {updated} files.")
