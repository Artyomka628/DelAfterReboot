import os
import uuid
import re
import sys

folder = os.path.dirname(os.path.abspath(__file__))

def new_guid():
    return str(uuid.uuid4()).upper()

new_version = None
if len(sys.argv) > 1:
    arg_value = sys.argv[1].strip()
    if arg_value and arg_value.lower() != "null":
        new_version = arg_value

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
