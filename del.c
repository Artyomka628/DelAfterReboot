#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include <shellapi.h>
#include <time.h>

typedef struct {
    wchar_t** items;
    size_t count;
    size_t capacity;
} PathList;

void pathlist_init(PathList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void pathlist_add(PathList* list, const wchar_t* path) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 256 : list->capacity * 2;
        list->items = (wchar_t**)realloc(list->items, new_cap * sizeof(wchar_t*));
        list->capacity = new_cap;
    }
    list->items[list->count++] = _wcsdup(path);
}

void pathlist_free(PathList* list) {
    for (size_t i = 0; i < list->count; i++)
        free(list->items[i]);
    free(list->items);
}

int is_admin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(
        &ntAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroup)) {

        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin;
}

void count_paths_bottom_up(const wchar_t* root, size_t* total) {
    wchar_t search[MAX_PATH];
    swprintf(search, MAX_PATH, L"%s\\*", root);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 ||
            wcscmp(ffd.cFileName, L"..") == 0)
            continue;

        wchar_t full[MAX_PATH];
        swprintf(full, MAX_PATH, L"%s\\%s", root, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            count_paths_bottom_up(full, total);
            (*total)++;
        } else {
            (*total)++;
        }

    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

void collect_paths_bottom_up(const wchar_t* root, PathList* list) {
    wchar_t search[MAX_PATH];
    swprintf(search, MAX_PATH, L"%s\\*", root);

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(search, &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (wcscmp(ffd.cFileName, L".") == 0 ||
            wcscmp(ffd.cFileName, L"..") == 0)
            continue;

        wchar_t full[MAX_PATH];
        swprintf(full, MAX_PATH, L"%s\\%s", root, ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            collect_paths_bottom_up(full, list);
            pathlist_add(list, full);
        } else {
            pathlist_add(list, full);
        }

    } while (FindNextFileW(hFind, &ffd));

    FindClose(hFind);
}

int write_pending_delete(PathList* list) {
    HKEY hKey;
    if (RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
        0,
        KEY_QUERY_VALUE | KEY_SET_VALUE,
        &hKey) != ERROR_SUCCESS)
        return 0;

    wchar_t* existing = NULL;
    DWORD type = 0, size = 0;

    if (RegQueryValueExW(
        hKey,
        L"PendingFileRenameOperations",
        NULL,
        &type,
        NULL,
        &size) == ERROR_SUCCESS && type == REG_MULTI_SZ) {

        existing = (wchar_t*)malloc(size);
        RegQueryValueExW(
            hKey,
            L"PendingFileRenameOperations",
            NULL,
            NULL,
            (LPBYTE)existing,
            &size);
    }

    size_t total_needed = (list->count * 2 + 1) * MAX_PATH;
    wchar_t* buffer = (wchar_t*)calloc(total_needed, sizeof(wchar_t));
    wchar_t* out = buffer;

    if (existing) {
        wchar_t* p = existing;
        while (*p) {
            size_t len = wcslen(p) + 1;
            wcscpy(out, p);
            out += len;
            p += len;
        }
        *out++ = L'\0';
        free(existing);
    }

    for (size_t i = 0; i < list->count; i++) {
        wcscpy(out, L"\\??\\");
        wcscat(out, list->items[i]);
        out += wcslen(out) + 1;
        *out++ = L'\0';

        double percent = ((double)(i + 1) / list->count) * 100.0;
        printf("\rMarked: %zu / %zu (%.1f%%)", i + 1, list->count, percent);
        fflush(stdout);
    }

    *out = L'\0';
    printf("\n");

    int ok = RegSetValueExW(
        hKey,
        L"PendingFileRenameOperations",
        0,
        REG_MULTI_SZ,
        (BYTE*)buffer,
        (DWORD)((out - buffer + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;

    free(buffer);
    RegCloseKey(hKey);
    return ok;
}

int main(int argc, char** argv) {
    if (!is_admin()) {
        printf("Error: administrator privileges are required.\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: del.exe <folder_path>\n");
        return 2;
    }

    wchar_t root[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, root, MAX_PATH);

    DWORD attr = GetFileAttributesW(root);
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        printf("Error: specified path does not exist or is not a directory.\n");
        return 2;
    }

    clock_t start = clock();

    size_t total = 1;
    count_paths_bottom_up(root, &total);

    printf("Total items: %zu\n", total);

    PathList list;
    pathlist_init(&list);
    collect_paths_bottom_up(root, &list);
    pathlist_add(&list, root);

    if (!write_pending_delete(&list)) {
        printf("Error while writing to registry.\n");
        pathlist_free(&list);
        return 3;
    }

    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;

    printf("Done.\n");
    printf("Execution time: %.2f seconds\n", elapsed);

    pathlist_free(&list);
    return 0;
}

__attribute__((section(".rsrc$01"), used))
static const char manifest[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
"  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">"
"    <security>"
"      <requestedPrivileges>"
"        <requestedExecutionLevel level=\"requireAdministrator\" uiAccess=\"false\"/>"
"      </requestedPrivileges>"
"    </security>"
"  </trustInfo>"
"</assembly>";

