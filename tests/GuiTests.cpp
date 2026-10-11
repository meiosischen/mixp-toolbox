#include "MenuRegistry.h"
#include "resource.h"
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

using namespace mixp;
static DWORD targetPid = 0;
static HWND found = NULL;
static int groups = 0;
static void require(bool condition, const char *description) {
    if (!condition) { printf("FAIL: %s\n", description); throw Error(L"GUI test assertion failed."); }
}
static void pass(const char *description) { ++groups; printf("PASS: %s\n", description); }
static BOOL CALLBACK findMain(HWND window, LPARAM) {
    DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    if (pid == targetPid && IsWindowVisible(window) && GetDlgItem(window, IDC_LIST)) { found = window; return FALSE; }
    return TRUE;
}
static BOOL CALLBACK findConfirm(HWND window, LPARAM) {
    DWORD pid = 0; GetWindowThreadProcessId(window, &pid);
    if (pid == targetPid && IsWindowVisible(window) && GetDlgItem(window, IDYES) && !GetDlgItem(window, IDC_LIST)) {
        found = window; return FALSE;
    }
    return TRUE;
}
static HWND waitWindow(bool confirmation) {
    for (unsigned i = 0; i < 150; ++i) {
        found = NULL; EnumWindows(confirmation ? findConfirm : findMain, 0);
        if (found) return found;
        Sleep(100);
    }
    throw Error(L"Timed out waiting for test application window.");
}
static HWND control(HWND window, int id) { return GetDlgItem(window, id); }
static void text(HWND window, int id, const std::wstring &value) {
    SendMessageW(control(window, id), WM_SETTEXT, 0, (LPARAM)value.c_str());
}
static std::wstring text(HWND window, int id) {
    wchar_t buffer[32768] = {0};
    SendMessageW(control(window, id), WM_GETTEXT, 32768, (LPARAM)buffer);
    return std::wstring(buffer);
}
static void click(HWND window, int id) {
    require(IsWindowEnabled(control(window, id)) != FALSE, "clicked control is enabled");
    PostMessageW(control(window, id), BM_CLICK, 0, 0);
}
static void combo(HWND window, int id, int index) {
    SendMessageW(control(window, id), CB_SETCURSEL, index, 0);
    SendMessageW(window, WM_COMMAND, MAKEWPARAM(id, CBN_SELCHANGE), (LPARAM)control(window, id));
}
static void waitRows(HWND window, int expected) {
    for (unsigned i = 0; i < 100; ++i) {
        if (SendMessageW(control(window, IDC_LIST), LVM_GETITEMCOUNT, 0, 0) == expected) return;
        Sleep(100);
    }
    throw Error(L"Timed out waiting for refreshed table rows.");
}
static void waitEditor(HWND window, bool creating) {
    for (unsigned i = 0; i < 100; ++i) {
        if ((IsWindowEnabled(control(window, IDC_VERB)) != FALSE) == creating &&
            (IsWindowEnabled(control(window, IDC_LABEL)) != FALSE)) return;
        Sleep(100);
    }
    throw Error(L"Timed out waiting for editor mode.");
}
static void waitLabel(HWND window, const std::wstring &label) {
    for (unsigned i = 0; i < 100; ++i) {
        if (text(window, IDC_LABEL) == label) return;
        Sleep(100);
    }
    throw Error(L"Timed out waiting for the selected menu editor.");
}
static void capture(HWND window, const wchar_t *filename) {
    require(SendMessageW(window, WM_APP + 101, 0, 0) != FALSE, "capture actual GUI state");
    require(MoveFileExW(L"build\\Release\\gui-test.bmp", filename, MOVEFILE_REPLACE_EXISTING) != FALSE,
            "preserve captured GUI state");
}
static Entry waitEntry(RegistryStore &store, Source source, const wchar_t *object,
                       const wchar_t *verb, const std::wstring &label) {
    for (unsigned i = 0; i < 100; ++i) {
        try { Entry entry = store.read(source, object, verb); if (entry.label == label) return entry; }
        catch (const Error &) {}
        Sleep(100);
    }
    throw Error(L"Timed out waiting for GUI registry save.");
}

static int startupSmoke() {
    STARTUPINFOW startup; PROCESS_INFORMATION process;
    ZeroMemory(&startup, sizeof(startup)); ZeroMemory(&process, sizeof(process)); startup.cb = sizeof(startup);
    wchar_t command[] = L"build\\Release\\mixp-toolbox.exe";
    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) return 1;
    targetPid = process.dwProcessId;
    int result = 1;
    try {
        HWND window = waitWindow(false);
        require(IsWindowUnicode(window) && text(window, IDC_SAVE) == L"\x4fdd\x5b58", "production GUI initializes");
        PostMessageW(window, WM_CLOSE, 0, 0);
        require(WaitForSingleObject(process.hProcess, 10000) == WAIT_OBJECT_0, "production GUI closes");
        DWORD code = 1; GetExitCodeProcess(process.hProcess, &code); require(code == 0, "production exit status");
        printf("PASS: shipped Release executable starts and closes; no menu changes made.\n");
        result = 0;
    } catch (...) { printf("FAIL: production executable startup smoke\n"); }
    if (WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 1);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    return result;
}

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--production-smoke")) return startupSmoke();
    wchar_t suffix[100], temp[MAX_PATH];
    wsprintfW(suffix, L"Software\\MixpToolboxTests\\Gui-%lu-%lu", GetCurrentProcessId(), GetTickCount());
    GetTempPathW(MAX_PATH, temp);
    std::wstring base = suffix;
    RegistryStore store(HKEY_CURRENT_USER, base + L"\\User\\Classes", HKEY_CURRENT_USER,
        base + L"\\Machine\\Classes", std::wstring(temp) + L"MixpToolboxGuiBackups");
    std::wstring command = L"\"build\\Release\\mixp-toolbox-test.exe\" \"" + base + L"\"";
    std::vector<wchar_t> buffer(command.begin(), command.end()); buffer.push_back(0);
    STARTUPINFOW startup; PROCESS_INFORMATION process;
    ZeroMemory(&startup, sizeof(startup)); ZeroMemory(&process, sizeof(process)); startup.cb = sizeof(startup);
    if (!CreateProcessW(NULL, &buffer[0], NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process)) {
        printf("ERROR: cannot launch test application (%lu)\n", GetLastError()); return 1;
    }
    targetPid = process.dwProcessId;
    int result = 1; HWND window = NULL;
    try {
        window = waitWindow(false);
        require(IsWindowUnicode(window) && IsWindowUnicode(control(window, IDC_LABEL)), "Unicode window and editor");
        require(text(window, IDC_SAVE) == L"\x4fdd\x5b58", "Chinese button caption");
        pass("native Unicode GUI starts with Chinese labels");

        waitRows(window, 0);
        require(IsWindowVisible(control(window, IDC_EMPTY)) && !IsWindowEnabled(control(window, IDC_LABEL)),
                "empty table explains the next action and editor awaits New");
        click(window, IDC_REFRESH); Sleep(200);
        require(IsWindowVisible(control(window, IDC_EMPTY)), "refresh preserves an explicit empty-state message");
        capture(window, L"build\\Release\\gui-empty.bmp");
        Draft fixture; fixture.source = AllUsers; fixture.object = L"Directory"; fixture.verb = L"SameVerb";
        fixture.label = L"Folder fixture"; fixture.command = L"notepad.exe \"%1\""; store.create(fixture);
        fixture.source = CurrentUser; fixture.object = L"Mixp.GuiType"; fixture.label = L"Type fixture"; store.create(fixture);
        click(window, IDC_REFRESH); waitRows(window, 2);
        require(!IsWindowVisible(control(window, IDC_EMPTY)) && store.list(L"*", -1).empty(),
                "refresh overview finds folders and ProgIDs when all-files has no entries");
        capture(window, L"build\\Release\\gui-overview.bmp");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, base + L"\\Machine\\Classes\\Directory");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, base + L"\\User\\Classes\\Mixp.GuiType");
        click(window, IDC_REFRESH); waitRows(window, 0);
        pass("GUI refresh has explicit empty feedback and discovers static menus across types");

        click(window, IDC_NEW); waitEditor(window, true);
        require(SendMessageW(control(window, IDC_SCOPE), CB_GETCURSEL, 0, 0) == AllFiles &&
            text(window, IDC_SAVE) == L"\x6dfb\x52a0\x83dc\x5355" &&
            text(window, IDC_LOCATION).find(L"\x65b0\x589e\x6a21\x5f0f") != std::wstring::npos &&
            text(window, IDC_ARGUMENTS) == L"\"%1\"", "New activates an explicit editor and concrete target");
        require(!IsWindowEnabled(control(window, IDC_SAVE)) && store.listAll(-1).empty(),
                "New does not write an incomplete registry item");
        capture(window, L"build\\Release\\gui-new.bmp");
        pass("first New click enables editor and displays Add Menu feedback");

        std::wstring label = L"\x7528 JOE \x7f16\x8f91", program = L"C:\\Program Files\\\x7f16\x8f91\x5668\\joe.exe";
        std::wstring args = L"-colors darkblue2 -tab 4 \"%1\"";
        text(window, IDC_VERB, L"GuiOpen"); text(window, IDC_LABEL, label);
        text(window, IDC_PROGRAM, program); text(window, IDC_ARGUMENTS, args);
        require(text(window, IDC_COMMAND) == buildCommand(program, args), "command preview quotes executable and retains parameters");
        click(window, IDC_SAVE);
        Entry original = waitEntry(store, CurrentUser, L"*", L"GuiOpen", label);
        require(original.command == buildCommand(program, args), "GUI creates exact command");
        pass("GUI creates a menu with Chinese path and default arguments");

        // Wait for refresh/selection after save before changing the editor again.
        waitEditor(window, false);
        waitRows(window, 1);
        text(window, IDC_LABEL, L"\x66f4\x65b0\x540d\x79f0"); click(window, IDC_SAVE);
        Entry updated = waitEntry(store, CurrentUser, L"*", L"GuiOpen", L"\x66f4\x65b0\x540d\x79f0");
        require(updated.command == original.command, "label-only edit preserves command bytes");
        pass("GUI modifies the display name without rewriting arguments");

        Sleep(200);
        text(window, IDC_SEARCH, L"nothing matches");
        require(SendMessageW(control(window, IDC_LIST), LVM_GETITEMCOUNT, 0, 0) == 0, "search hides unmatched entries");
        text(window, IDC_SEARCH, L"gUiOpEn");
        require(SendMessageW(control(window, IDC_LIST), LVM_GETITEMCOUNT, 0, 0) == 1, "search matches ID without case sensitivity");
        text(window, IDC_SEARCH, L"");
        pass("GUI live search filters the saved list");

        fixture.source = CurrentUser; fixture.object = L"Directory"; fixture.verb = L"GuiOpen";
        fixture.label = L"Directory same ID"; store.create(fixture);
        combo(window, IDC_SCOPE, AllScopes); waitRows(window, 2);
        text(window, IDC_SEARCH, L"Directory"); waitRows(window, 1);
        SendMessageW(control(window, IDC_LIST), WM_KEYDOWN, VK_HOME, 1);
        waitLabel(window, fixture.label);
        text(window, IDC_LABEL, L"Directory updated"); click(window, IDC_SAVE);
        waitEntry(store, CurrentUser, L"Directory", L"GuiOpen", L"Directory updated");
        waitEditor(window, false); waitRows(window, 2);
        require(text(window, IDC_LOCATION).find(L"\\Directory\\") != std::wstring::npos &&
            text(window, IDC_LABEL) == L"Directory updated" &&
            store.read(CurrentUser, L"*", L"GuiOpen").label == L"\x66f4\x65b0\x540d\x79f0",
            "overview save retains the exact object when IDs match across types");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, base + L"\\User\\Classes\\Directory");
        combo(window, IDC_SCOPE, AllFiles); waitRows(window, 1);
        SendMessageW(control(window, IDC_LIST), WM_KEYDOWN, VK_HOME, 1);
        waitLabel(window, L"\x66f4\x65b0\x540d\x79f0");
        pass("overview searches types and edits the correct target with duplicate menu IDs");

        click(window, IDC_ENABLED); Sleep(100); click(window, IDC_EXTENDED); Sleep(100); click(window, IDC_SAVE);
        for (unsigned i = 0; i < 100; ++i) {
            updated = store.read(CurrentUser, L"*", L"GuiOpen");
            if (!updated.enabled && updated.extended) break;
            Sleep(100);
        }
        require(!updated.enabled && updated.extended, "GUI updates enabled and Shift flags");
        Sleep(300); require(SendMessageW(window, WM_APP + 101, 0, 0) != FALSE, "in-process test-window capture");
        pass("GUI saves enable/Shift flags; captures the actual window");

        combo(window, IDC_VIEW_SOURCE, 1); Sleep(200);
        click(window, IDC_NEW); waitEditor(window, true);
        text(window, IDC_SEARCH, L"nothing matches");
        combo(window, IDC_SOURCE, AllUsers);
        text(window, IDC_VERB, L"GuiMachine"); text(window, IDC_LABEL, L"Machine example");
        text(window, IDC_PROGRAM, L"C:\\Windows\\notepad.exe"); click(window, IDC_SAVE);
        updated = waitEntry(store, AllUsers, L"*", L"GuiMachine", L"Machine example");
        require(store.list(L"*", -1).size() == 2, "both sources shown independently");
        waitEditor(window, false); waitRows(window, 2);
        require(text(window, IDC_SEARCH).empty() && SendMessageW(control(window, IDC_VIEW_SOURCE), CB_GETCURSEL, 0, 0) == 0,
                "successful create clears search and adjusts an excluding source filter");
        Sleep(300); click(window, IDC_DELETE);
        HWND confirmation = waitWindow(true); PostMessageW(control(confirmation, IDYES), BM_CLICK, 0, 0);
        bool removed = false;
        for (unsigned i = 0; i < 100; ++i) {
            try { store.read(AllUsers, L"*", L"GuiMachine"); }
            catch (const Error &failure) { if (failure.code == ERROR_FILE_NOT_FOUND) { removed = true; break; } }
            Sleep(100);
        }
        require(removed && store.list(L"*", CurrentUser).size() == 1, "GUI delete affects only selected branch");
        pass("GUI source selection and confirmed deletion preserve the other source");

        Sleep(200); PostMessageW(window, WM_CLOSE, 0, 0);
        require(WaitForSingleObject(process.hProcess, 10000) == WAIT_OBJECT_0, "GUI closes normally");
        DWORD code = 1; GetExitCodeProcess(process.hProcess, &code); require(code == 0, "GUI exits with zero status");
        printf("RESULT: %d GUI groups passed; application writes isolated under Software\\MixpToolboxTests.\n", groups);
        result = 0;
    } catch (const Error &failure) {
        char detail[1500]; WideCharToMultiByte(CP_UTF8, 0, failure.message.c_str(), -1, detail, sizeof(detail), NULL, NULL);
        printf("ERROR: %s (%lu)\n", detail, failure.code);
    } catch (...) { printf("ERROR: unexpected GUI test exception\n"); }
    if (WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0) TerminateProcess(process.hProcess, 1);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    try { RegistryStore::deleteTree(HKEY_CURRENT_USER, base); } catch (...) { result = 1; }
    wchar_t pattern[100]; wsprintfW(pattern, L"\\menu-*-%lu-*.reg", targetPid);
    std::wstring folder = std::wstring(temp) + L"MixpToolboxGuiBackups";
    WIN32_FIND_DATAW data; HANDLE find = FindFirstFileW((folder + pattern).c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do { DeleteFileW((folder + L"\\" + data.cFileName).c_str()); } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    return result;
}
