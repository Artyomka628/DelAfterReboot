import os
import sys
import time
import ctypes
import winreg
import builtins

_print_log = []
_use_debug = False

def _print_hook(*args, **kwargs):
    text = " ".join(str(a) for a in args)
    _print_log.append(text)
    if _use_debug:
        ctypes.windll.user32.MessageBoxW(0, text, "DEBUG LOG", 0x40)

def show_log(title="DEBUG LOG"):
    text = "\n".join(_print_log) if _print_log else "Log is empty"
    ctypes.windll.user32.MessageBoxW(0, text, title, 0x40)

def ensure_admin():
    try:
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
    except:
        is_admin = False

    if not is_admin:
        params = " ".join(f'"{arg}"' for arg in sys.argv[1:])
        ctypes.windll.shell32.ShellExecuteW(
            None,
            "runas",
            sys.executable,
            params,
            None,
            1
        )
        sys.exit(0)

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
    global _use_debug

    ensure_admin()
    
    if not is_admin():
        print("Error: administrator privileges are required.")
        if _use_debug:
            show_log()
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Usage: python del.py <folder_path> [--debug]")
        print("--debug - debug print with MessageBox")
        print("errorlevel values:")
        print("0 - success, 1 - not an admin, 2 - wrong path, 3 - registry write error")
        if _use_debug:
            show_log()
        sys.exit(2)

    folder = sys.argv[1]

    if len(sys.argv) > 2 and sys.argv[2] == "--debug":
        _use_debug = True
        builtins.print = _print_hook

    if not os.path.isdir(folder):
        print("Error: specified path does not exist or is not a directory.")
        if _use_debug:
            show_log()
        sys.exit(2)

    start_time = time.time()

    try:
        paths = collect_paths_bottom_up(folder)
        write_pending_delete(paths)
    except Exception as e:
        print(f"Error while writing to registry: {e}")
        if _use_debug:
            show_log()
        sys.exit(3)

    elapsed = time.time() - start_time

    print(f"Marked items: {len(paths)}")
    print(f"Execution time: {elapsed:.2f} seconds")

    if _use_debug:
        show_log()

    sys.exit(0)

if __name__ == "__main__":
    main()
