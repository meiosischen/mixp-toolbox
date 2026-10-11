#include "MenuRegistry.h"
#include <stdio.h>
#include <cwchar>

using namespace mixp;

static int passed = 0;
static void require(bool condition, const char *description) {
    if (!condition) { printf("FAIL: %s\n", description); throw Error(L"Test assertion failed."); }
}
static void pass(const char *description) { ++passed; printf("PASS: %s\n", description); }
static void raw(HKEY root, const std::wstring &path, const wchar_t *name,
                DWORD type, const void *bytes, DWORD size) {
    HKEY key = NULL;
    LONG status = RegCreateKeyExW(root, path.c_str(), 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (!status) { status = RegSetValueExW(key, name, 0, type, (const BYTE *)bytes, size); RegCloseKey(key); }
    if (status) throw Error(L"Cannot set test fixture.", status);
}
static void stringAt(HKEY root, const std::wstring &path, const wchar_t *name, const wchar_t *text) {
    raw(root, path, name, REG_SZ, text, (DWORD)((wcslen(text) + 1) * 2));
}
static bool exists(HKEY root, const std::wstring &path) {
    HKEY key = NULL; LONG code = RegOpenKeyExW(root, path.c_str(), 0, KEY_READ, &key);
    if (!code) RegCloseKey(key);
    return code == ERROR_SUCCESS;
}
static bool validBackup(const std::wstring &path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    BYTE bom[2] = {0}; DWORD size = 0;
    bool good = ReadFile(file, bom, 2, &size, NULL) && size == 2 && bom[0] == 255 && bom[1] == 254;
    CloseHandle(file); return good;
}
static void importBackup(const std::wstring &path) {
    std::wstring command = L"regedit.exe /S \"" + path + L"\"";
    std::vector<wchar_t> buffer(command.begin(), command.end()); buffer.push_back(0);
    STARTUPINFOW startup; PROCESS_INFORMATION process;
    ZeroMemory(&startup, sizeof(startup)); ZeroMemory(&process, sizeof(process)); startup.cb = sizeof(startup);
    if (!CreateProcessW(NULL, &buffer[0], NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process))
        throw Error(L"Cannot run regedit import.", GetLastError());
    DWORD wait = WaitForSingleObject(process.hProcess, 15000), code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    if (wait == WAIT_TIMEOUT) TerminateProcess(process.hProcess, 1);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    require(wait == WAIT_OBJECT_0 && code == 0, "regedit import exits successfully");
}
static Draft draft(Source source, const wchar_t *verb) {
    Draft d; d.source = source; d.object = L"*"; d.verb = verb;
    d.label = L"\x4f7f\x7528 JOE \x7f16\x8f91";
    d.command = buildCommand(L"C:\\Program Files\\\x7f16\x8f91\x5668\\joe.exe", L"-colors darkblue2 -tab 4 \"%1\"");
    return d;
}
static void cleanFiles(const std::wstring &folder) {
    WIN32_FIND_DATAW data; HANDLE find = FindFirstFileW((folder + L"\\*.reg").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do { DeleteFileW((folder + L"\\" + data.cFileName).c_str()); } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    RemoveDirectoryW(folder.c_str());
}

int main() {
    wchar_t suffix[80], temp[MAX_PATH];
    wsprintfW(suffix, L"Software\\MixpToolboxTests\\Registry-%lu-%lu", GetCurrentProcessId(), GetTickCount());
    GetTempPathW(MAX_PATH, temp);
    std::wstring base = suffix, user = base + L"\\User\\Classes", machine = base + L"\\Machine\\Classes";
    std::wstring backups = std::wstring(temp) + L"MixpRegistryTests-";
    wchar_t pid[30]; wsprintfW(pid, L"%lu", GetCurrentProcessId()); backups += pid;
    RegistryStore store(HKEY_CURRENT_USER, user, HKEY_LOCAL_MACHINE, machine, backups);
    int result = 1;
    try {
        require(store.listAll(-1).empty(), "empty Classes roots return an empty overview");
        require(store.objectFor(AllFiles, L"") == L"*" && store.objectFor(Directories, L"") == L"Directory" &&
            store.objectFor(Drives, L"") == L"Drive" && store.objectFor(FileSystemObjects, L"") == L"AllFilesystemObjects", "scope mappings");
        stringAt(HKEY_LOCAL_MACHINE, machine + L"\\.mixptest", L"", L"Mixp.MachineType");
        require(store.objectFor(Extension, L" .mixptest ") == L"Mixp.MachineType", "machine file association");
        stringAt(HKEY_CURRENT_USER, user + L"\\.mixptest", L"", L"Mixp.UserType");
        require(store.objectFor(Extension, L".mixptest") == L"Mixp.UserType", "user association priority");
        bool rejected = false;
        try { std::wstring ignored = store.objectFor(Extension, L".notregistered"); } catch (const Error &) { rejected = true; }
        require(rejected && !exists(HKEY_CURRENT_USER, user + L"\\.notregistered"), "unknown extension leaves associations unchanged");
        pass("five scopes and extension resolution without association changes");

        Draft d = draft(CurrentUser, L"MixpOpen"); store.create(d);
        Draft other = draft(AllUsers, L"MixpOpen"); other.label = L"Machine copy"; store.create(other);
        Entry original = store.read(CurrentUser, L"*", L"MixpOpen");
        require(original.label == d.label && original.command == d.command && original.editable, "Unicode strings preserved");
        require(store.list(L"*", -1).size() == 2 && store.list(L"*", 0).size() == 1 && store.list(L"*", 1).size() == 1, "both roots separately listed");
        rejected = false; try { store.create(d); } catch (const Error &) { rejected = true; }
        require(rejected && sameTree(original.snapshot, store.read(CurrentUser, L"*", L"MixpOpen").snapshot), "duplicate never overwrites");
        pass("Unicode create/read, duplicate rejection and HKCU/HKLM separation");

        Draft folder = draft(AllUsers, L"OpenFolder"); folder.object = L"Folder"; store.create(folder);
        Draft type = draft(CurrentUser, L"OpenType"); type.object = L"Mixp.UserType"; store.create(type);
        stringAt(HKEY_CURRENT_USER, user + L"\\CLSID\\Example\\shellex\\ContextMenuHandlers", L"", L"handler");
        std::vector<Entry> overview = store.listAll(-1);
        require(overview.size() == 4 && store.listAll(CurrentUser).size() == 2 &&
                store.listAll(AllUsers).size() == 2, "overview enumerates types and separates sources");
        require(overview[2].object == L"Folder" && overview[3].object == L"Mixp.UserType",
                "overview includes folders and ProgIDs with stable target identity");
        RegistryStore::deleteTree(HKEY_LOCAL_MACHINE, machine + L"\\Folder");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, user + L"\\Mixp.UserType");
        pass("all-scope overview finds static menus outside the all-files branch");

        std::wstring key = user + L"\\*\\shell\\MixpOpen";
        DWORD number = 0xdeadbeef;
        raw(HKEY_CURRENT_USER, key, L"UnknownNumber", REG_DWORD, &number, sizeof(number));
        BYTE binary[120]; for (unsigned i = 0; i < sizeof(binary); ++i) binary[i] = (BYTE)i;
        raw(HKEY_CURRENT_USER, key + L"\\Extra\\Nested", L"Binary", REG_BINARY, binary, sizeof(binary));
        const wchar_t multi[] = L"one\0\x4e8c\0";
        raw(HKEY_CURRENT_USER, key, L"Multi", REG_MULTI_SZ, multi, sizeof(multi));
        stringAt(HKEY_CURRENT_USER, key, L"Quoted\\\"Name", L"Text with \\ and \"quotes\"");
        original = store.read(CurrentUser, L"*", L"MixpOpen");
        Entry before = original;
        d.label = L"\x66f4\x65b0\x540d\x79f0"; d.enabled = false; d.extended = true; d.expandEnvironment = true;
        d.command = L"\"%SystemRoot%\\notepad.exe\" \"%1\"";
        std::wstring saved = store.update(original, d);
        Entry after = store.read(CurrentUser, L"*", L"MixpOpen");
        require(!after.enabled && after.extended && after.commandType == REG_EXPAND_SZ && after.label == d.label, "flags and expand-string type saved");
        require(validBackup(saved) && exists(HKEY_CURRENT_USER, key + L"\\Extra\\Nested"), "backup BOM and preserved nested metadata");
        // Import over an edited branch: newly added flags must disappear too.
        importBackup(saved);
        require(sameTree(before.snapshot, store.read(CurrentUser, L"*", L"MixpOpen").snapshot), "backup import restores exact prior values, flags and subtree");
        pass("update with automatic backup; real regedit restores all metadata and flags");

        original = store.read(CurrentUser, L"*", L"MixpOpen");
        std::wstring exported = backups + L"\\\x5bfc\x51fa.reg";
        store.exportEntry(original, exported);
        require(validBackup(exported), "Unicode export filename");
        saved = store.remove(original);
        require(!exists(HKEY_CURRENT_USER, key) && exists(HKEY_LOCAL_MACHINE, machine + L"\\*\\shell\\MixpOpen"), "delete limited to selected source");
        importBackup(exported);
        require(sameTree(original.snapshot, store.read(CurrentUser, L"*", L"MixpOpen").snapshot), "export/import exact round trip including binary continuation and multi-string");
        pass("recursive delete/export/import round trip with Unicode, binary and REG_MULTI_SZ");

        original = store.read(CurrentUser, L"*", L"MixpOpen");
        stringAt(HKEY_CURRENT_USER, key, L"ExternalChange", L"do not overwrite");
        rejected = false; try { std::wstring ignored = store.update(original, d); } catch (const Error &) { rejected = true; }
        require(rejected, "stale update rejected");
        rejected = false; try { std::wstring ignored = store.remove(original); } catch (const Error &) { rejected = true; }
        require(rejected && exists(HKEY_CURRENT_USER, key), "stale delete rejected");
        pass("external registry changes block stale writes and deletes");

        original = store.read(CurrentUser, L"*", L"MixpOpen");
        std::wstring blocker = std::wstring(temp) + L"MixpBackupBlocker-" + pid;
        HANDLE file = CreateFileW(blocker.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        require(file != INVALID_HANDLE_VALUE, "backup failure fixture"); CloseHandle(file);
        RegistryStore broken(HKEY_CURRENT_USER, user, HKEY_LOCAL_MACHINE, machine, blocker);
        rejected = false; try { std::wstring ignored = broken.update(original, d); } catch (const Error &) { rejected = true; }
        require(rejected && sameTree(original.snapshot, store.read(CurrentUser, L"*", L"MixpOpen").snapshot), "backup failure blocks update");
        rejected = false; try { std::wstring ignored = broken.remove(original); } catch (const Error &) { rejected = true; }
        require(rejected && exists(HKEY_CURRENT_USER, key), "backup failure blocks delete"); DeleteFileW(blocker.c_str());
        pass("unwritable backup location prevents mutation");

        stringAt(HKEY_CURRENT_USER, key + L"\\ddeexec", L"", L"[open(\"%1\")]");
        original = store.read(CurrentUser, L"*", L"MixpOpen");
        require(!original.editable, "DDE not treated as a static command");
        rejected = false; try { std::wstring ignored = store.update(original, d); } catch (const Error &) { rejected = true; }
        require(rejected, "DDE edit blocked");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, key + L"\\ddeexec");
        stringAt(HKEY_CURRENT_USER, key + L"\\command", L"DelegateExecute", L"{test-handler}");
        require(!store.read(CurrentUser, L"*", L"MixpOpen").editable, "handler edit blocked");
        RegistryStore::deleteTree(HKEY_CURRENT_USER, key + L"\\command");
        stringAt(HKEY_CURRENT_USER, key + L"\\DropTarget", L"CLSID", L"{drop-handler}");
        stringAt(HKEY_CURRENT_USER, key + L"\\command", L"", L"notepad.exe \"%1\"");
        require(!store.read(CurrentUser, L"*", L"MixpOpen").editable, "DropTarget edit blocked");
        stringAt(HKEY_CURRENT_USER, user + L"\\*\\shell\\WithoutCommand", L"", L"No command");
        require(!store.read(CurrentUser, L"*", L"WithoutCommand").editable, "missing command read safely");
        pass("DDE/handler/missing-command entries stay read-only");

        Draft invalid = draft(CurrentUser, L"bad\\child");
        rejected = false; try { store.create(invalid); } catch (const Error &) { rejected = true; }
        require(rejected && !exists(HKEY_CURRENT_USER, user + L"\\*\\shell\\bad"), "invalid nested ID rejected");
        invalid = draft(CurrentUser, L"bad]ID");
        rejected = false; try { store.create(invalid); } catch (const Error &) { rejected = true; }
        require(rejected, "ID must support subsequent .reg backup");
        invalid = draft(CurrentUser, L"Empty"); invalid.command = L" ";
        rejected = false; try { store.create(invalid); } catch (const Error &) { rejected = true; }
        require(rejected, "empty command rejected");
        std::wstring exe, args;
        require(splitCommand(draft(CurrentUser, L"x").command, exe, args) && exe == L"C:\\Program Files\\\x7f16\x8f91\x5668\\joe.exe", "quoted command parsed");
        require(!splitCommand(L"C:\\Program Files\\joe.exe \"%1\"", exe, args), "ambiguous unquoted command uses raw editor");
        require(containsInsensitive(L"MixpOpen", L"OPEN"), "case insensitive search");
        pass("input validation, quoting, raw-command fallback and case-insensitive search");

        printf("RESULT: %d groups passed; all writes isolated under Software\\MixpToolboxTests.\n", passed);
        result = 0;
    } catch (const Error &failure) {
        char text[1500]; WideCharToMultiByte(CP_UTF8, 0, failure.message.c_str(), -1, text, sizeof(text), NULL, NULL);
        printf("ERROR: %s (code %lu)\n", text, failure.code);
    } catch (...) { printf("ERROR: unexpected test exception\n"); }
    try { RegistryStore::deleteTree(HKEY_CURRENT_USER, base); RegistryStore::deleteTree(HKEY_LOCAL_MACHINE, base); }
    catch (...) { printf("ERROR: test registry cleanup failed\n"); result = 1; }
    cleanFiles(backups);
    return result;
}
