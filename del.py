import os
import sys
import time
import ctypes
import winreg


def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        return False


def collect_paths_bottom_up(root_path):
    files = []
    dirs = []

    for current_root, dirnames, filenames in os.walk(root_path, topdown=False):
        for name in filenames:
            files.append(os.path.join(current_root, name))
        for name in dirnames:
            dirs.append(os.path.join(current_root, name))

    dirs.append(root_path)
    return files + dirs


def write_pending_delete(paths):
    key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager"

    reg_key = winreg.OpenKey(
        winreg.HKEY_LOCAL_MACHINE,
        key_path,
        0,
        winreg.KEY_SET_VALUE | winreg.KEY_QUERY_VALUE
    )

    try:
        existing, _ = winreg.QueryValueEx(
            reg_key,
            "PendingFileRenameOperations"
        )
    except FileNotFoundError:
        existing = []

    new_entries = existing[:]

    for path in paths:
        nt_path = r"\??\\" + os.path.abspath(path)
        new_entries.append(nt_path)
        new_entries.append("")

    winreg.SetValueEx(
        reg_key,
        "PendingFileRenameOperations",
        0,
        winreg.REG_MULTI_SZ,
        new_entries
    )

    winreg.CloseKey(reg_key)


def main():
    if not is_admin():
        print("Error: administrator privileges are required.")
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Usage: python del.py <folder_path>")
        print("errorlevel values:")
        print("0 - sucess, 1 - not an admin, 2 - wrong path, 3 - registry write error")
        sys.exit(2)

    folder = sys.argv[1]

    if not os.path.isdir(folder):
        print("Error: specified path does not exist or is not a directory.")
        sys.exit(2)

    start_time = time.time()

    try:
        paths = collect_paths_bottom_up(folder)
        write_pending_delete(paths)
    except Exception as e:
        print(f"Error while writing to registry: {e}")
        sys.exit(3)

    elapsed = time.time() - start_time

    print(f"Marked items: {len(paths)}")
    print(f"Execution time: {elapsed:.2f} seconds")

    sys.exit(0)


if __name__ == "__main__":
    main()
