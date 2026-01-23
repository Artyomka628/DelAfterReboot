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

def show_log(title="DEBUG LOG"):
    text = "\n".join(_print_log) if _print_log else "Log is empty"
    try:
        ctypes.windll.user32.MessageBoxW(0, "\n".join(sys.argv), "Arguments | " + title , 0x40)
        ctypes.windll.user32.MessageBoxW(0, text, title, 0x40)
    except Exception:
        print(text)

def ensure_admin():
    try:
        is_admin = ctypes.windll.shell32.IsUserAnAdmin()
    except Exception:
        is_admin = False

    if not is_admin:
        script = os.path.abspath(sys.argv[0])
        try:
            exe_abs = os.path.abspath(sys.executable)
            if exe_abs.lower() == script.lower() or getattr(sys, "frozen", False):
                params = " ".join(f'"{arg}"' for arg in sys.argv[1:])
            else:
                params = " ".join(f'"{arg}"' for arg in [script] + sys.argv[1:])
        except Exception:
            params = " ".join(f'"{arg}"' for arg in [script] + sys.argv[1:])

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
        return bool(ctypes.windll.shell32.IsUserAnAdmin())
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

def _format_seconds(s):
    s = max(0.0, float(s))
    if s >= 3600:
        h = int(s // 3600)
        m = int((s % 3600) // 60)
        sec = int(s % 60)
        return f"{h}:{m:02d}:{sec:02d}"
    elif s >= 60:
        m = int(s // 60)
        sec = int(s % 60)
        return f"{m:02d}:{sec:02d}"
    else:
        return f"{s:.1f}s"

def write_pending_delete(paths, show_progress=False):
    key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager"
    access = winreg.KEY_SET_VALUE | winreg.KEY_QUERY_VALUE
    try:
        access |= winreg.KEY_WOW64_64KEY
    except AttributeError:
        pass

    reg_key = winreg.OpenKey(
        winreg.HKEY_LOCAL_MACHINE,
        key_path,
        0,
        access
    )

    try:
        existing, _ = winreg.QueryValueEx(
            reg_key,
            "PendingFileRenameOperations"
        )
    except FileNotFoundError:
        existing = []

    new_entries = list(existing) if isinstance(existing, list) else []

    count = len(paths)
    if count == 0:
        count = 1

    start_time = time.time()
    bar_length = 25

    if show_progress:
        sys.stdout.write("\n")
        sys.stdout.flush()

    for i, path in enumerate(paths, 1):
        abs_path = os.path.abspath(path)
        nt_path = r"\??\{}".format(abs_path)
        new_entries.append(nt_path)
        new_entries.append("")

        if show_progress:
            elapsed = time.time() - start_time
            avg = elapsed / i if i else 0
            remain = avg * (len(paths) - i)
            rem_str = _format_seconds(remain)

            frac = i / len(paths)
            filled = int(frac * bar_length)
            bar = "=" * filled + " " * (bar_length - filled)
            percent = frac * 100

            line = f"\r{i}/{len(paths)} [{bar}] {percent:3.0f}% Remaining: {rem_str}"
            sys.stdout.write(line)
            sys.stdout.flush()

    if show_progress:
        sys.stdout.write("\n")
        sys.stdout.flush()

    winreg.SetValueEx(
        reg_key,
        "PendingFileRenameOperations",
        0,
        winreg.REG_MULTI_SZ,
        new_entries
    )

    winreg.CloseKey(reg_key)

def clear_pending_delete():
    key_path = r"SYSTEM\CurrentControlSet\Control\Session Manager"
    access = winreg.KEY_SET_VALUE | winreg.KEY_QUERY_VALUE
    try:
        access |= winreg.KEY_WOW64_64KEY
    except AttributeError:
        pass

    reg_key = winreg.OpenKey(
        winreg.HKEY_LOCAL_MACHINE,
        key_path,
        0,
        access
    )

    try:
        winreg.DeleteValue(reg_key, "PendingFileRenameOperations")
        print("PendingFileRenameOperations cleared.")
    except FileNotFoundError:
        print("No PendingFileRenameOperations value present.")
    except OSError as e:
        print(f"Error while clearing PendingFileRenameOperations: {e}")
    finally:
        try:
            winreg.CloseKey(reg_key)
        except Exception:
            pass

def main():
    global _use_debug

    ensure_admin()

    if not is_admin():
        print("Error: administrator privileges are required.")
        if _use_debug:
            show_log()
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Usage: python del.py <folder_path> [--debug]  or  python del.py --cancel [--debug]")
        print("--debug - debug print with MessageBox")
        print("--cancel - clear PendingFileRenameOperations (cancel pending deletes)")
        sys.exit(2)

    if sys.argv[1] == "--cancel":
        if len(sys.argv) > 2 and sys.argv[2] == "--debug":
            _use_debug = True
            builtins.print = _print_hook

        try:
            clear_pending_delete()
        except Exception as e:
            print(f"Error while clearing registry: {e}")
            if _use_debug:
                show_log()
            sys.exit(3)

        if _use_debug:
            show_log()
        sys.exit(0)

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
        show_progress = (not _use_debug)
        write_pending_delete(paths, show_progress=show_progress)
    except Exception as e:
        print(f"Error while writing to registry: {e}")
        if _use_debug:
            show_log()
        sys.exit(3)

    elapsed = time.time() - start_time

    print("\n" + "=" * 17)
    print(f"Marked: {len(paths)} files")
    print(f"Elapsed Time: {elapsed:.2f} seconds")
    print("=" * 17)

    if _use_debug:
        show_log()

    sys.exit(0)

if __name__ == "__main__":
    main()