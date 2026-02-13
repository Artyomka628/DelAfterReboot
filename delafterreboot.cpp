#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "version.h"

namespace {
std::vector<std::wstring> g_print_log;
bool g_use_debug = false;

#ifndef APP_VERSION
#define APP_VERSION "unknown"
#endif
#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif
#ifndef BUILD_DATE
#define BUILD_DATE "unknown"
#endif

static std::wstring widen(const char *s) {
    std::wstring out;
    while (s && *s) {
        out.push_back(static_cast<wchar_t>(*s++));
    }
    return out;
}

void PrintLine(const std::wstring &text);

static void PrintVersion() {
    PrintLine(L"Version: " + widen(APP_VERSION));
    PrintLine(L"Commit:  " + widen(GIT_COMMIT_HASH));
    PrintLine(L"Built:   " + widen(BUILD_DATE));
}

static void PrintHelp() {
    PrintLine(L"Usage:");
    PrintLine(L"  delafterreboot.exe <folder_path> [--debug] [--uac]");
    PrintLine(L"  delafterreboot.exe --cancel [--debug] [--uac]");
    PrintLine(L"");
    PrintLine(L"Options:");
    PrintLine(L"  --help      Show this help");
    PrintLine(L"  --version   Show version information");
    PrintLine(L"  --debug     Debug output using MessageBox");
    PrintLine(L"  --cancel    Clear PendingFileRenameOperations");
    PrintLine(L"  --uac       Request elevation via UAC");
    PrintLine(L"  --nodelay   Skip 1 second delay before exit");
    PrintLine(L"");
    PrintVersion();
}

void PrintLine(const std::wstring &text) {
    if (g_use_debug) {
        g_print_log.push_back(text);
        return;
    }
    std::wcout << text << std::endl;
}

std::wstring JoinLines(const std::vector<std::wstring> &lines) {
    std::wstring joined;
    for (size_t i = 0; i < lines.size(); ++i) {
        joined += lines[i];
        if (i + 1 < lines.size()) {
            joined += L"\n";
        }
    }
    return joined;
}

void ShowLog(const std::vector<std::wstring> &args, const std::wstring &title = L"DEBUG LOG") {
    std::wstring text = JoinLines(g_print_log);
    if (text.empty()) {
        text = L"Log is empty";
    }
    std::wstring args_text = JoinLines(args);

    std::wstring args_title = L"Arguments | " + title;
    int args_result = MessageBoxW(nullptr, args_text.c_str(), args_title.c_str(), MB_ICONINFORMATION);
    int log_result = MessageBoxW(nullptr, text.c_str(), title.c_str(), MB_ICONINFORMATION);
    if (args_result == 0 || log_result == 0) {
        std::wcerr << text << std::endl;
    }
}

std::wstring GetModulePath() {
    std::wstring path(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (size == 0) {
        return L"";
    }
    if (size >= path.size()) {
        path.resize(size + MAX_PATH);
        size = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (size == 0) {
            return L"";
        }
    }
    path.resize(size);
    return path;
}

bool IsAdmin() {
    BOOL is_admin = FALSE;
    is_admin = IsUserAnAdmin();
    return is_admin != FALSE;
}

bool EnsureAdmin(const std::vector<std::wstring> &args, bool force_uac) {
    if (IsAdmin()) {
        return true;
    }

    if (!force_uac) {
        return false;
    }

    std::wstring exe_path = GetModulePath();
    if (exe_path.empty()) {
        exe_path = args.empty() ? L"" : args.front();
    }

    std::wstring params;
    bool first = true;
    for (const auto &arg : args) {
        if (arg == L"--uac") {
            continue;
        }
        if (!first) {
            params += L" ";
        }
        params += L"\"" + arg + L"\"";
        first = false;
    }

    HINSTANCE result = ShellExecuteW(nullptr, L"runas", exe_path.c_str(), params.c_str(), nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<intptr_t>(result) <= 32) {
        std::wstringstream error;
        error << L"Failed to request elevation: " << reinterpret_cast<intptr_t>(result);
        PrintLine(error.str());
        return false;
    }

    ExitProcess(0);
    return false;
}

std::wstring FormatSeconds(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }
    std::wstringstream out;
    if (seconds >= 3600.0) {
        int h = static_cast<int>(seconds / 3600.0);
        int m = static_cast<int>((seconds - h * 3600) / 60.0);
        int sec = static_cast<int>(seconds) % 60;
        out << h << L":" << std::setw(2) << std::setfill(L'0') << m << L":" << std::setw(2) << sec;
    } else if (seconds >= 60.0) {
        int m = static_cast<int>(seconds / 60.0);
        int sec = static_cast<int>(seconds) % 60;
        out << std::setw(2) << std::setfill(L'0') << m << L":" << std::setw(2) << sec;
    } else {
        out << std::fixed << std::setprecision(1) << seconds << L"s";
    }
    return out.str();
}

void CollectPathsRecursive(const std::wstring &root_path,
                           std::vector<std::wstring> &files,
                           std::vector<std::wstring> &dirs) {
    std::wstring search_path = root_path;
    if (!search_path.empty() && search_path.back() != L'\\') {
        search_path += L"\\";
    }
    search_path += L"*";

    WIN32_FIND_DATAW find_data;
    HANDLE handle = FindFirstFileExW(
        search_path.c_str(),
        FindExInfoBasic,
        &find_data,
        FindExSearchNameMatch,
        nullptr,
        FIND_FIRST_EX_LARGE_FETCH
    );
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    std::vector<std::wstring> local_dirs;

    do {
        std::wstring name = find_data.cFileName;
        if (name == L"." || name == L"..") {
            continue;
        }
        std::wstring full_path = root_path;
        if (!full_path.empty() && full_path.back() != L'\\') {
            full_path += L"\\";
        }
        full_path += name;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            CollectPathsRecursive(full_path, files, dirs);
            local_dirs.push_back(full_path);
        } else {
            files.push_back(full_path);
        }
    } while (FindNextFileW(handle, &find_data));

    FindClose(handle);

    for (const auto &dir : local_dirs) {
        dirs.push_back(dir);
    }
}

std::vector<std::wstring> CollectPathsBottomUp(const std::wstring &root_path) {
    std::vector<std::wstring> files;
    std::vector<std::wstring> dirs;
    CollectPathsRecursive(root_path, files, dirs);
    dirs.push_back(root_path);

    std::vector<std::wstring> all;
    all.reserve(files.size() + dirs.size());
    all.insert(all.end(), files.begin(), files.end());
    all.insert(all.end(), dirs.begin(), dirs.end());
    return all;
}

bool QueryMultiString(HKEY key, const std::wstring &value_name, std::vector<std::wstring> &values) {
    DWORD type = 0;
    DWORD data_size = 0;
    LSTATUS status = RegQueryValueExW(key, value_name.c_str(), nullptr, &type, nullptr, &data_size);
    if (status == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    if (status != ERROR_SUCCESS || type != REG_MULTI_SZ || data_size == 0) {
        return false;
    }

    std::vector<wchar_t> buffer(data_size / sizeof(wchar_t));
    status = RegQueryValueExW(key, value_name.c_str(), nullptr, &type,
                              reinterpret_cast<LPBYTE>(buffer.data()), &data_size);
    if (status != ERROR_SUCCESS || type != REG_MULTI_SZ) {
        return false;
    }

    const wchar_t *ptr = buffer.data();
    while (*ptr != L'\0') {
        std::wstring entry(ptr);
        values.push_back(entry);
        ptr += entry.size() + 1;
    }

    return true;
}

void WritePendingDelete(const std::vector<std::wstring> &paths, bool show_progress) {
    const wchar_t *key_path = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
    REGSAM access = KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_WOW64_64KEY;
    HKEY reg_key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key_path, 0, access, &reg_key) != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to open registry key");
    }

    std::vector<std::wstring> entries;
    QueryMultiString(reg_key, L"PendingFileRenameOperations", entries);

    size_t total = paths.size();
    if (total == 0) {
        total = 1;
    }

    auto start_time = std::chrono::steady_clock::now();
    const int bar_length = 25;

    auto last_draw = start_time;
    size_t index = 0;
    for (const auto &path : paths) {
        ++index;

        std::wstring nt_path = L"\\\\??\\" + path;
        entries.push_back(nt_path);
        entries.push_back(L"");

        auto now = std::chrono::steady_clock::now();
        if (show_progress &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_draw).count() >= 80) {
            last_draw = now;

            auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - start_time);
            double avg = elapsed.count() / static_cast<double>(index);
            double remain = avg * static_cast<double>(paths.size() - index);
            std::wstring rem_str = FormatSeconds(remain);

            double frac = static_cast<double>(index) / static_cast<double>(paths.size());
            int filled = static_cast<int>(frac * bar_length);
            std::wstring bar(filled, L'=');
            bar += std::wstring(bar_length - filled, L' ');
            int percent = static_cast<int>(frac * 100.0 + 0.5);

            std::wstringstream line;
            line << L"\r" << index << L"/" << paths.size() << L" [" << bar << L"] "
                 << std::setw(3) << percent << L"% Remaining: " << rem_str;
            std::wcout << line.str() << std::flush;
        }
    }

    if (show_progress) {
        std::wcout << L"\n" << std::flush;
    }

    size_t total_chars = 1;
    for (const auto &entry : entries) {
        total_chars += entry.size() + 1;
    }

    std::vector<wchar_t> buffer;
    buffer.reserve(total_chars);
    for (const auto &entry : entries) {
        buffer.insert(buffer.end(), entry.begin(), entry.end());
        buffer.push_back(L'\0');
    }
    buffer.push_back(L'\0');

    RegSetValueExW(reg_key, L"PendingFileRenameOperations", 0, REG_MULTI_SZ,
                   reinterpret_cast<const BYTE *>(buffer.data()),
                   static_cast<DWORD>(buffer.size() * sizeof(wchar_t)));

    RegCloseKey(reg_key);
}

void ClearPendingDelete() {
    const wchar_t *key_path = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager";
    REGSAM access = KEY_SET_VALUE | KEY_QUERY_VALUE | KEY_WOW64_64KEY;
    HKEY reg_key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, key_path, 0, access, &reg_key) != ERROR_SUCCESS) {
        throw std::runtime_error("Failed to open registry key");
    }

    LSTATUS status = RegDeleteValueW(reg_key, L"PendingFileRenameOperations");
    if (status == ERROR_FILE_NOT_FOUND) {
        PrintLine(L"No PendingFileRenameOperations value present.");
    } else if (status == ERROR_SUCCESS) {
        PrintLine(L"PendingFileRenameOperations cleared.");
    } else {
        std::wstringstream error;
        error << L"Error while clearing PendingFileRenameOperations: " << status;
        PrintLine(error.str());
    }

    RegCloseKey(reg_key);
}
}

int wmain(int argc, wchar_t *argv[]) {
    std::vector<std::wstring> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    std::vector<std::wstring> flags;
    if (argc > 1) {
        flags.assign(args.begin() + 1, args.end());
    }

    if (std::find(flags.begin(), flags.end(), L"--help") != flags.end()) {
        PrintHelp();
        return 0;
    }

    if (std::find(flags.begin(), flags.end(), L"--version") != flags.end()) {
        PrintVersion();
        return 0;
    }

    bool force_uac = std::find(flags.begin(), flags.end(), L"--uac") != flags.end();
    bool debug_flag = std::find(flags.begin(), flags.end(), L"--debug") != flags.end();
    bool no_delay = std::find(flags.begin(), flags.end(), L"--nodelay") != flags.end();

    if (debug_flag) {
        g_use_debug = true;
    }

    if (!EnsureAdmin(flags, force_uac)) {
        PrintLine(L"Error: administrator privileges are required.");
        PrintLine(L"Use --uac to request elevation.");
        if (g_use_debug) {
            ShowLog(args);
        }
        return 1;
    }

    if (flags.empty()) {
        PrintHelp();
        return 0;
    }

    if (std::find(flags.begin(), flags.end(), L"--cancel") != flags.end()) {
        try {
            ClearPendingDelete();
        } catch (const std::exception &ex) {
            std::wstringstream error;
            error << L"Error while clearing registry: " << ex.what();
            PrintLine(error.str());
            if (g_use_debug) {
                ShowLog(args);
            }
            if (!no_delay) Sleep(1000);
            return 3;
        }

        if (g_use_debug) {
            ShowLog(args);
        }
        if (!no_delay) Sleep(1000);
        return 0;
    }

    std::wstring folder;
    for (const auto &arg : flags) {
        if (arg.rfind(L"--", 0) != 0) {
            folder = arg;
            break;
        }
    }

    if (folder.empty()) {
        PrintLine(L"Error: no folder specified.");
        if (!no_delay) Sleep(1000);
        return 2;
    }

    DWORD attrs = GetFileAttributesW(folder.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        PrintLine(L"Error: specified path does not exist or is not a directory.");
        if (g_use_debug) {
            ShowLog(args);
        }
        if (!no_delay) Sleep(1000);
        return 2;
    }

    auto start_time = std::chrono::steady_clock::now();

    std::vector<std::wstring> paths;
    try {
        paths = CollectPathsBottomUp(folder);
        bool show_progress = !g_use_debug;
        WritePendingDelete(paths, show_progress);
    } catch (const std::exception &ex) {
        std::wstringstream error;
        error << L"Error while writing to registry: " << ex.what();
        PrintLine(error.str());
        if (g_use_debug) {
            ShowLog(args);
        }
        if (!no_delay) Sleep(1000);
        return 3;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(
        std::chrono::steady_clock::now() - start_time);

    PrintLine(L"=================");

    {
        std::wstringstream line;
        line << L"Marked: " << paths.size() << L" files";
        PrintLine(line.str());
    }
    {
        std::wstringstream line;
        line << L"Elapsed Time: " << std::fixed << std::setprecision(2) << elapsed.count() << L" seconds";
        PrintLine(line.str());
    }
    PrintLine(L"=================");

    if (g_use_debug) {
        ShowLog(args);
    }

    if (!no_delay) Sleep(1000);
    return 0;
}