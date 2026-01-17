import os
import uuid
import re

folder = os.path.dirname(os.path.abspath(__file__))

def new_guid():
    return str(uuid.uuid4()).upper()

new_version = input("Enter new version (leave empty to keep current): ").strip()

for filename in os.listdir(folder):
    if filename.endswith(".wxs"):
        path = os.path.join(folder, filename)
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()

        product_id = new_guid()
        content = re.sub(
            r'(<Product[^>]*Id=")[^"]*(")',
            lambda m: m.group(1) + product_id + m.group(2),
            content
        )

        if new_version:
            content = re.sub(
                r'(<Product[^>]*Version=")[^"]*(")',
                lambda m: m.group(1) + new_version + m.group(2),
                content
            )

        with open(path, "w", encoding="utf-8") as f:
            f.write(content)

        print(f"{filename}: {product_id}")

input()