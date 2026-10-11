#include "MenuRegistry.h"
#include <algorithm>
#include <cwchar>
#include <sstream>
#include <iomanip>

namespace mixp {
namespace {

struct Key {
    HKEY handle;
    Key() : handle(NULL) {}
    ~Key() { if (handle) RegCloseKey(handle); }
private:
    Key(const Key &);
    Key &operator=(const Key &);
};

void check(LONG status, const wchar_t *message) {
    if (status != ERROR_SUCCESS) throw Error(message, status);
}

bool missing(LONG code) { return code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND; }

bool lessName(const Value &a, const Value &b) { return a.name < b.name; }
bool lessChild(const Tree &a, const Tree &b) { return a.name < b.name; }

Tree snapshot(HKEY key, unsigned depth = 0) {
    if (depth > 32) throw Error(L"\x6ce8\x518c\x8868\x5b50\x9879\x5c42\x7ea7\x8fc7\x6df1\xff0c\x65e0\x6cd5\x5bfc\x51fa\x3002");
    Tree tree;
    DWORD maxName = 0, maxData = 0, maxSubkey = 0;
    check(RegQueryInfoKeyW(key, NULL, NULL, NULL, NULL, &maxSubkey, NULL,
                          NULL, &maxName, &maxData, NULL, NULL), L"\x65e0\x6cd5\x8bfb\x53d6\x6ce8\x518c\x8868\x9879\x4fe1\x606f\x3002");
    if (maxData > 4 * 1024 * 1024) throw Error(L"\x6ce8\x518c\x8868\x503c\x8fc7\x5927\xff0c\x65e0\x6cd5\x5bfc\x51fa\x3002");
    std::vector<wchar_t> name(maxName + 2);
    std::vector<unsigned char> data(maxData + 2);
    for (DWORD i = 0;; ++i) {
        DWORD n = (DWORD)name.size(), size = (DWORD)data.size(), type = 0;
        LONG status = RegEnumValueW(key, i, &name[0], &n, NULL, &type, &data[0], &size);
        if (status == ERROR_NO_MORE_ITEMS) break;
        check(status, L"\x65e0\x6cd5\x5217\x51fa\x6ce8\x518c\x8868\x503c\x3002");
        Value value;
        value.name.assign(&name[0], n);
        value.type = type;
        value.bytes.assign(data.begin(), data.begin() + size);
        tree.values.push_back(value);
    }
    name.resize(maxSubkey + 2);
    for (DWORD i = 0;; ++i) {
        DWORD n = (DWORD)name.size();
        LONG status = RegEnumKeyExW(key, i, &name[0], &n, NULL, NULL, NULL, NULL);
        if (status == ERROR_NO_MORE_ITEMS) break;
        check(status, L"\x65e0\x6cd5\x5217\x51fa\x6ce8\x518c\x8868\x5b50\x9879\x3002");
        Key child;
        check(RegOpenKeyExW(key, &name[0], 0, KEY_READ, &child.handle), L"\x65e0\x6cd5\x8bfb\x53d6\x6ce8\x518c\x8868\x5b50\x9879\x3002");
        Tree node = snapshot(child.handle, depth + 1);
        node.name.assign(&name[0], n);
        tree.children.push_back(node);
    }
    std::sort(tree.values.begin(), tree.values.end(), lessName);
    std::sort(tree.children.begin(), tree.children.end(), lessChild);
    return tree;
}

const Value *findValue(const Tree &tree, const wchar_t *name) {
    for (size_t i = 0; i < tree.values.size(); ++i)
        if (!lstrcmpiW(tree.values[i].name.c_str(), name)) return &tree.values[i];
    return NULL;
}

const Tree *findChild(const Tree &tree, const wchar_t *name) {
    for (size_t i = 0; i < tree.children.size(); ++i)
        if (!lstrcmpiW(tree.children[i].name.c_str(), name)) return &tree.children[i];
    return NULL;
}

bool stringValue(const Value *value, std::wstring &text) {
    if (!value || (value->type != REG_SZ && value->type != REG_EXPAND_SZ) ||
        value->bytes.size() % sizeof(wchar_t)) return false;
    // Decode explicitly; Registry values are UTF-16, including in an ANSI BCB6 build.
    text.clear();
    for (size_t i = 0; i + 1 < value->bytes.size(); i += 2) {
        wchar_t c = (wchar_t)(value->bytes[i] | (value->bytes[i + 1] << 8));
        if (!c) {
            // Embedded NULs in a command must not be silently discarded on save.
            for (size_t j = i; j < value->bytes.size(); ++j)
                if (value->bytes[j]) return false;
            return true;
        }
        text += c;
    }
    return true;
}

void component(const std::wstring &name) {
    if (name.empty() || name.size() > 255 || name.find_first_of(L"\\\r\n") != std::wstring::npos)
        throw Error(L"\x5185\x90e8\x6807\x8bc6\x4e0d\x80fd\x4e3a\x7a7a\x3001\x4e0d\x80fd\x8d85\x8fc7 255 \x5b57\x7b26\xff0c\x4e5f\x4e0d\x80fd\x5305\x542b\x53cd\x659c\x6760\x6216\x6362\x884c\x3002");
}

void validObject(const std::wstring &object) {
    component(object);
}

void validate(const Draft &draft) {
    validObject(draft.object);
    component(draft.verb);
    if (draft.object.find_first_of(L"[]") != std::wstring::npos || draft.verb.find_first_of(L"[]") != std::wstring::npos)
        throw Error(L"\x5185\x90e8\x6807\x8bc6\x6216\x6587\x4ef6\x7c7b\x578b\x540d\x4e0d\x80fd\x5305\x542b\x65b9\x62ec\x53f7\xff0c\x4ee5\x4fbf\x4fdd\x5b58 .reg \x5907\x4efd\x3002");
    if (mixp::trim(draft.command).empty()) throw Error(L"\x8bf7\x586b\x5199\x542f\x52a8\x547d\x4ee4\x3002");
    if (draft.command.find_first_of(L"\r\n") != std::wstring::npos ||
        draft.label.find_first_of(L"\r\n") != std::wstring::npos)
        throw Error(L"\x83dc\x5355\x540d\x79f0\x548c\x542f\x52a8\x547d\x4ee4\x4e0d\x80fd\x5305\x542b\x6362\x884c\x3002");
}

void setString(HKEY key, const wchar_t *name, const std::wstring &text, DWORD type) {
    check(RegSetValueExW(key, name, 0, type, (const BYTE *)text.c_str(),
                        (DWORD)((text.size() + 1) * sizeof(wchar_t))), L"\x65e0\x6cd5\x4fdd\x5b58\x6ce8\x518c\x8868\x6587\x672c\x3002");
}

void eraseValue(HKEY key, const wchar_t *name) {
    LONG status = RegDeleteValueW(key, name);
    if (!missing(status)) check(status, L"\x65e0\x6cd5\x5220\x9664\x6ce8\x518c\x8868\x503c\x3002");
}

void flag(HKEY key, const wchar_t *name, bool present) {
    DWORD type = 0;
    LONG status = RegQueryValueExW(key, name, NULL, &type, NULL, NULL);
    if (present) {
        if (missing(status)) setString(key, name, L"", REG_SZ);
        else check(status, L"\x65e0\x6cd5\x8bfb\x53d6\x83dc\x5355\x72b6\x6001\x3002");
    } else eraseValue(key, name);
}

void apply(HKEY key, const Draft &draft, DWORD labelType) {
    if (draft.label.empty()) eraseValue(key, L"");
    else setString(key, L"", draft.label, labelType);
    Key command;
    check(RegCreateKeyExW(key, L"command", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                          &command.handle, NULL), L"\x65e0\x6cd5\x6253\x5f00\x83dc\x5355\x547d\x4ee4\x3002");
    setString(command.handle, L"", draft.command, draft.expandEnvironment ? REG_EXPAND_SZ : REG_SZ);
    flag(key, L"LegacyDisable", !draft.enabled);
    flag(key, L"Extended", draft.extended);
}

void restoreValue(HKEY key, const Tree &before, const wchar_t *name) {
    const Value *value = findValue(before, name);
    if (value) {
        const BYTE *bytes = value->bytes.empty() ? NULL : &value->bytes[0];
        check(RegSetValueExW(key, name, 0, value->type, bytes, (DWORD)value->bytes.size()),
              L"\x65e0\x6cd5\x6062\x590d\x6ce8\x518c\x8868\x503c\xff0c\x8bf7\x5bfc\x5165\x5df2\x4fdd\x5b58\x7684\x5907\x4efd\x3002");
    } else eraseValue(key, name);
}

void restoreTree(HKEY key, const Tree &tree) {
    for (size_t i = 0; i < tree.values.size(); ++i) {
        const Value &v = tree.values[i];
        check(RegSetValueExW(key, v.name.c_str(), 0, v.type,
                            v.bytes.empty() ? NULL : &v.bytes[0], (DWORD)v.bytes.size()),
              L"\x65e0\x6cd5\x6062\x590d\x83dc\x5355\x9879\xff0c\x8bf7\x5bfc\x5165\x5df2\x4fdd\x5b58\x7684\x5907\x4efd\x3002");
    }
    for (size_t i = 0; i < tree.children.size(); ++i) {
        Key child;
        check(RegCreateKeyExW(key, tree.children[i].name.c_str(), 0, NULL, 0, KEY_READ | KEY_WRITE,
                              NULL, &child.handle, NULL), L"\x65e0\x6cd5\x6062\x590d\x6ce8\x518c\x8868\x5b50\x9879\xff0c\x8bf7\x5bfc\x5165\x5df2\x4fdd\x5b58\x7684\x5907\x4efd\x3002");
        restoreTree(child.handle, tree.children[i]);
    }
}

std::wstring quoted(const std::wstring &s) {
    std::wstring out = L"\"";
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\\' || s[i] == L'"') out += L'\\';
        out += s[i];
    }
    return out + L"\"";
}

void exportTree(std::wostringstream &out, const std::wstring &path, const Tree &tree) {
    if (path.find_first_of(L"\r\n[]") != std::wstring::npos)
        throw Error(L"\x6b64\x952e\x540d\x542b\x6709 .reg \x6587\x4ef6\x4e0d\x80fd\x8868\x793a\x7684\x5b57\x7b26\xff0c\x65e0\x6cd5\x5bfc\x51fa\x3002");
    out << L"[" << path << L"]\r\n";
    for (size_t i = 0; i < tree.values.size(); ++i) {
        const Value &v = tree.values[i];
        if (v.name.find_first_of(L"\r\n") != std::wstring::npos)
            throw Error(L"\x6b64\x503c\x540d\x542b\x6709 .reg \x6587\x4ef6\x4e0d\x80fd\x8868\x793a\x7684\x5b57\x7b26\xff0c\x65e0\x6cd5\x5bfc\x51fa\x3002");
        if (v.name.empty()) out << L"@";
        else out << quoted(v.name);
        out << L"=";
        std::wstring text;
        bool simpleString = v.type == REG_SZ && stringValue(&v, text) &&
                            !v.bytes.empty() && v.bytes[v.bytes.size() - 1] == 0 &&
                            v.bytes.size() == (text.size() + 1) * 2 &&
                            text.find_first_of(L"\r\n") == std::wstring::npos;
        if (simpleString) out << quoted(text);
        else if (v.type == REG_DWORD && v.bytes.size() == 4) {
            DWORD n = v.bytes[0] | (v.bytes[1] << 8) | (v.bytes[2] << 16) | ((DWORD)v.bytes[3] << 24);
            out << L"dword:" << std::hex << std::setw(8) << std::setfill(L'0') << n;
        } else {
            if (v.type == REG_BINARY) out << L"hex:";
            else out << L"hex(" << std::hex << v.type << L"):";
            for (size_t j = 0; j < v.bytes.size(); ++j) {
                if (j) out << L",";
                if (j && j % 24 == 0) out << L"\\\r\n  ";
                out << std::hex << std::setw(2) << std::setfill(L'0') << (unsigned)v.bytes[j];
            }
        }
        out << L"\r\n";
    }
    out << L"\r\n";
    for (size_t i = 0; i < tree.children.size(); ++i)
        exportTree(out, path + L"\\" + tree.children[i].name, tree.children[i]);
}

void writeRegFile(const std::wstring &filename, const std::wstring &contents, bool overwrite) {
    HANDLE file = CreateFileW(filename.c_str(), GENERIC_WRITE, 0, NULL,
                              overwrite ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) throw Error(L"\x65e0\x6cd5\x5199\x5165\x5907\x4efd\x6216\x5bfc\x51fa\x6587\x4ef6\x3002", GetLastError());
    DWORD written = 0;
    const unsigned char bom[] = {0xff, 0xfe};
    BOOL ok = WriteFile(file, bom, 2, &written, NULL) && written == 2;
    DWORD size = (DWORD)(contents.size() * sizeof(wchar_t));
    if (ok) ok = WriteFile(file, contents.c_str(), size, &written, NULL) && written == size;
    DWORD error = GetLastError();
    CloseHandle(file);
    if (!ok) {
        DeleteFileW(filename.c_str());
        throw Error(L"\x5907\x4efd\x6216\x5bfc\x51fa\x6587\x4ef6\x5199\x5165\x672a\x5b8c\x6210\x3002", error);
    }
}

void makeDirectories(const std::wstring &path) {
    if (path.empty()) throw Error(L"\x6ca1\x6709\x53ef\x5199\x5165\x7684\x5907\x4efd\x76ee\x5f55\x3002");
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        if (!(attrs & FILE_ATTRIBUTE_DIRECTORY)) throw Error(L"\x5907\x4efd\x4f4d\x7f6e\x4e0d\x662f\x76ee\x5f55\x3002");
        return;
    }
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos && slash > 2) makeDirectories(path.substr(0, slash));
    if (!CreateDirectoryW(path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        throw Error(L"\x65e0\x6cd5\x521b\x5efa\x5907\x4efd\x76ee\x5f55\x3002", GetLastError());
}

std::wstring backupDirectory() {
    wchar_t path[MAX_PATH] = {0};
    typedef HRESULT (WINAPI *GetFolder)(HWND, int, HANDLE, DWORD, LPWSTR);
    HMODULE shell = LoadLibraryW(L"shell32.dll");
    GetFolder folder = shell ? (GetFolder)GetProcAddress(shell, "SHGetFolderPathW") : NULL;
    HRESULT result = folder ? folder(NULL, 0x001c, NULL, 0, path) : E_FAIL;
    if (shell) FreeLibrary(shell);
    if (FAILED(result)) {
        DWORD n = GetEnvironmentVariableW(L"APPDATA", path, MAX_PATH);
        if (!n || n >= MAX_PATH) throw Error(L"\x65e0\x6cd5\x83b7\x53d6\x5f53\x524d\x7528\x6237\x7684\x5e94\x7528\x6570\x636e\x76ee\x5f55\x3002");
    }
    return std::wstring(path) + L"\\MixpToolbox\\backups";
}

bool lowerEntry(const Entry &a, const Entry &b) {
    int object = lstrcmpiW(a.object.c_str(), b.object.c_str());
    if (object) return object < 0;
    int name = lstrcmpiW(a.verb.c_str(), b.verb.c_str());
    return name ? name < 0 : a.source < b.source;
}

}

Error::Error(const std::wstring &text, DWORD status) : message(text), code(status) {
    if (status) {
        wchar_t buffer[1024] = {0};
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, status, 0, buffer, 1024, NULL);
        message += L"\r\n" + trim(buffer);
        if (status == ERROR_ACCESS_DENIED)
            message += L"\r\n\x4fee\x6539\x6240\x6709\x7528\x6237\x7684\x83dc\x5355\x9879\x9700\x8981\x6709\x5199\x5165\x6743\x9650\x7684\x8d26\x6237\xff0c\x901a\x5e38\x4e3a\x7ba1\x7406\x5458\x3002";
    }
}

Entry::Entry() : source(CurrentUser), labelType(REG_SZ), commandType(REG_SZ),
                  enabled(true), extended(false), editable(false) {}
Draft::Draft() : source(CurrentUser), enabled(true), extended(false), expandEnvironment(false) {}

RegistryStore::RegistryStore() : userRoot_(HKEY_CURRENT_USER), machineRoot_(HKEY_LOCAL_MACHINE),
    userPrefix_(L"Software\\Classes"), machinePrefix_(L"Software\\Classes"), backupDirectory_(backupDirectory()) {}

RegistryStore::RegistryStore(HKEY userRoot, const std::wstring &userPrefix,
    HKEY machineRoot, const std::wstring &machinePrefix, const std::wstring &backups)
    : userRoot_(userRoot), machineRoot_(machineRoot), userPrefix_(userPrefix),
      machinePrefix_(machinePrefix), backupDirectory_(backups) {}

HKEY RegistryStore::root(Source source) const { return source == CurrentUser ? userRoot_ : machineRoot_; }
std::wstring RegistryStore::prefix(Source source) const { return source == CurrentUser ? userPrefix_ : machinePrefix_; }

std::wstring RegistryStore::path(Source source, const std::wstring &object, const std::wstring &verb) const {
    validObject(object);
    if (!verb.empty()) component(verb);
    std::wstring result = prefix(source) + L"\\" + object + L"\\shell";
    if (!verb.empty()) result += L"\\" + verb;
    return result;
}

std::wstring RegistryStore::objectFor(Scope scope, const std::wstring &extension) const {
    switch (scope) {
    case AllFiles: return L"*";
    case Directories: return L"Directory";
    case Drives: return L"Drive";
    case FileSystemObjects: return L"AllFilesystemObjects";
    case Extension: break;
    default: throw Error(L"\x672a\x77e5\x83dc\x5355\x4f5c\x7528\x8303\x56f4\x3002");
    }
    std::wstring ext = trim(extension);
    if (ext.empty() || ext[0] != L'.' || ext.size() < 2 ||
        ext.find_first_of(L"\\/ :\r\n") != std::wstring::npos)
        throw Error(L"\x8bf7\x8f93\x5165\x5df2\x5173\x8054\x6587\x4ef6\x7c7b\x578b\x7684\x6269\x5c55\x540d\xff0c\x4f8b\x5982 .txt\x3002");
    component(ext);
    for (int source = CurrentUser; source <= AllUsers; ++source) {
        Key key;
        LONG status = RegOpenKeyExW(root((Source)source), (prefix((Source)source) + L"\\" + ext).c_str(),
                                    0, KEY_READ, &key.handle);
        if (missing(status)) continue;
        check(status, L"\x65e0\x6cd5\x8bfb\x53d6\x6587\x4ef6\x7c7b\x578b\x5173\x8054\x3002");
        Tree tree = snapshot(key.handle);
        std::wstring progid;
        if (stringValue(findValue(tree, L""), progid) && !progid.empty()) {
            component(progid);
            return progid;
        }
    }
    throw Error(L"\x6b64\x6269\x5c55\x540d\x5c1a\x672a\x5173\x8054\x6587\x4ef6\x7c7b\x578b\x3002\x8bf7\x9009\x62e9\x5df2\x6709\x6587\x4ef6\x5173\x8054\x7684\x6269\x5c55\x540d\xff0c\x672c\x5de5\x5177\x4e0d\x4f1a\x4fee\x6539\x6587\x4ef6\x5173\x8054\x3002");
}

std::vector<Entry> RegistryStore::list(const std::wstring &object, int sourceFilter) const {
    std::vector<Entry> result;
    for (int source = CurrentUser; source <= AllUsers; ++source) {
        if (sourceFilter >= 0 && sourceFilter != source) continue;
        Key key;
        LONG status = RegOpenKeyExW(root((Source)source), path((Source)source, object, L"").c_str(), 0,
                                    KEY_READ, &key.handle);
        if (missing(status)) continue;
        check(status, L"\x65e0\x6cd5\x5217\x51fa\x6b64\x8303\x56f4\x7684\x83dc\x5355\x9879\x3002");
        wchar_t name[256];
        for (DWORD i = 0;; ++i) {
            DWORD length = 256;
            status = RegEnumKeyExW(key.handle, i, name, &length, NULL, NULL, NULL, NULL);
            if (status == ERROR_NO_MORE_ITEMS) break;
            check(status, L"\x65e0\x6cd5\x5217\x51fa\x83dc\x5355\x9879\x3002");
            try { result.push_back(read((Source)source, object, std::wstring(name, length))); }
            catch (const Error &failure) { if (!missing(failure.code)) throw; }
        }
    }
    std::sort(result.begin(), result.end(), lowerEntry);
    return result;
}

std::vector<Entry> RegistryStore::listAll(int sourceFilter) const {
    std::vector<Entry> result;
    for (int source = CurrentUser; source <= AllUsers; ++source) {
        if (sourceFilter >= 0 && sourceFilter != source) continue;
        Key classes;
        LONG status = RegOpenKeyExW(root((Source)source), prefix((Source)source).c_str(),
                                    0, KEY_READ, &classes.handle);
        if (missing(status)) continue;
        check(status, L"\x65e0\x6cd5\x8bfb\x53d6\x83dc\x5355\x7c7b\x578b\x5217\x8868\x3002");
        wchar_t name[256];
        for (DWORD i = 0;; ++i) {
            DWORD length = 256;
            status = RegEnumKeyExW(classes.handle, i, name, &length, NULL, NULL, NULL, NULL);
            if (status == ERROR_NO_MORE_ITEMS) break;
            check(status, L"\x65e0\x6cd5\x5217\x51fa\x83dc\x5355\x7c7b\x578b\x3002");
            std::vector<Entry> items = list(std::wstring(name, length), source);
            result.insert(result.end(), items.begin(), items.end());
        }
    }
    std::sort(result.begin(), result.end(), lowerEntry);
    return result;
}

Entry RegistryStore::read(Source source, const std::wstring &object, const std::wstring &verb) const {
    Key key;
    check(RegOpenKeyExW(root(source), path(source, object, verb).c_str(), 0, KEY_READ, &key.handle),
          L"\x65e0\x6cd5\x8bfb\x53d6\x83dc\x5355\x9879\x3002");
    Entry entry;
    entry.source = source; entry.object = object; entry.verb = verb;
    entry.snapshot = snapshot(key.handle);
    const Value *label = findValue(entry.snapshot, L"");
    bool labelValid = !label || stringValue(label, entry.label);
    if (label) entry.labelType = label->type;
    const Tree *command = findChild(entry.snapshot, L"command");
    const Value *text = command ? findValue(*command, L"") : NULL;
    if (text) entry.commandType = text->type;
    entry.editable = labelValid && stringValue(text, entry.command) && !entry.command.empty() &&
        !findChild(entry.snapshot, L"ddeexec") && !findChild(entry.snapshot, L"DropTarget") && !findValue(entry.snapshot, L"MUIVerb") &&
        !findValue(entry.snapshot, L"SubCommands") && !findValue(entry.snapshot, L"ExplorerCommandHandler") &&
        !findValue(*command, L"DelegateExecute");
    entry.enabled = !findValue(entry.snapshot, L"LegacyDisable");
    entry.extended = findValue(entry.snapshot, L"Extended") != NULL;
    return entry;
}

void RegistryStore::checkCurrent(const Entry &entry) const {
    Entry latest = read(entry.source, entry.object, entry.verb);
    if (!sameTree(entry.snapshot, latest.snapshot))
        throw Error(L"\x6b64\x83dc\x5355\x9879\x5df2\x88ab\x5176\x4ed6\x7a0b\x5e8f\x4fee\x6539\xff0c\x8bf7\x5237\x65b0\x540e\x91cd\x65b0\x9009\x4e2d\x3002");
}

void RegistryStore::create(const Draft &draft) {
    validate(draft);
    if (draft.label.empty()) throw Error(L"\x65b0\x589e\x83dc\x5355\x9879\x5fc5\x987b\x586b\x5199\x83dc\x5355\x540d\x79f0\x3002");
    Key key;
    DWORD disposition = 0;
    const std::wstring target = path(draft.source, draft.object, draft.verb);
    check(RegCreateKeyExW(root(draft.source), target.c_str(), 0, NULL, 0, KEY_READ | KEY_WRITE,
                          NULL, &key.handle, &disposition), L"\x65e0\x6cd5\x521b\x5efa\x83dc\x5355\x9879\x3002");
    if (disposition != REG_CREATED_NEW_KEY)
        throw Error(L"\x6b64\x5185\x90e8\x6807\x8bc6\x5df2\x5b58\x5728\xff0c\x8bf7\x9009\x4e2d\x5df2\x6709\x9879\x8fdb\x884c\x7f16\x8f91\xff0c\x6216\x586b\x5199\x4e0d\x540c\x7684\x6807\x8bc6\x3002");
    try { apply(key.handle, draft, REG_SZ); }
    catch (...) {
        RegCloseKey(key.handle); key.handle = NULL;
        deleteTree(root(draft.source), target);
        throw;
    }
}

std::wstring RegistryStore::update(const Entry &original, const Draft &draft) {
    validate(draft);
    if (!original.editable) throw Error(L"\x6b64\x9879\x4f7f\x7528\x6269\x5c55\x5904\x7406\x7a0b\x5e8f\x6216 DDE\xff0c\x65e0\x6cd5\x4f5c\x4e3a\x666e\x901a\x542f\x52a8\x547d\x4ee4\x7f16\x8f91\x3002");
    if (original.source != draft.source || original.object != draft.object || original.verb != draft.verb)
        throw Error(L"\x5df2\x6709\x9879\x7684\x4fdd\x5b58\x4f4d\x7f6e\x548c\x5185\x90e8\x6807\x8bc6\x4e0d\x80fd\x76f4\x63a5\x66f4\x6539\xff0c\x8bf7\x53e6\x5efa\x83dc\x5355\x9879\x3002");
    checkCurrent(original);
    std::wstring saved = backup(original);
    Key key, command;
    check(RegOpenKeyExW(root(original.source), path(original.source, original.object, original.verb).c_str(),
                        0, KEY_READ | KEY_WRITE, &key.handle), L"\x65e0\x6cd5\x4fee\x6539\x83dc\x5355\x9879\x3002");
    // The command key is already present for an editable entry.
    check(RegOpenKeyExW(key.handle, L"command", 0, KEY_READ | KEY_WRITE, &command.handle),
          L"\x65e0\x6cd5\x4fee\x6539\x83dc\x5355\x547d\x4ee4\x3002");
    try { apply(key.handle, draft, original.labelType); }
    catch (...) {
        restoreValue(key.handle, original.snapshot, L"");
        restoreValue(key.handle, original.snapshot, L"LegacyDisable");
        restoreValue(key.handle, original.snapshot, L"Extended");
        restoreValue(command.handle, *findChild(original.snapshot, L"command"), L"");
        throw;
    }
    return saved;
}

std::wstring RegistryStore::remove(const Entry &original) {
    checkCurrent(original);
    std::wstring saved = backup(original);
    const std::wstring target = path(original.source, original.object, original.verb);
    try { deleteTree(root(original.source), target); }
    catch (...) {
        Key key;
        LONG code = RegCreateKeyExW(root(original.source), target.c_str(), 0, NULL, 0,
                                    KEY_READ | KEY_WRITE, NULL, &key.handle, NULL);
        if (!code) restoreTree(key.handle, original.snapshot);
        throw;
    }
    return saved;
}

void RegistryStore::deleteTree(HKEY rootKey, const std::wstring &target) {
    Key key;
    LONG status = RegOpenKeyExW(rootKey, target.c_str(), 0, KEY_READ | KEY_WRITE, &key.handle);
    if (missing(status)) return;
    check(status, L"\x65e0\x6cd5\x6253\x5f00\x5f85\x5220\x9664\x7684\x83dc\x5355\x9879\x3002");
    wchar_t child[256];
    for (;;) {
        DWORD size = 256;
        status = RegEnumKeyExW(key.handle, 0, child, &size, NULL, NULL, NULL, NULL);
        if (status == ERROR_NO_MORE_ITEMS) break;
        check(status, L"\x65e0\x6cd5\x8bfb\x53d6\x5f85\x5220\x9664\x7684\x5b50\x9879\x3002");
        deleteTree(key.handle, std::wstring(child, size));
    }
    RegCloseKey(key.handle); key.handle = NULL;
    check(RegDeleteKeyW(rootKey, target.c_str()), L"\x65e0\x6cd5\x5220\x9664\x83dc\x5355\x9879\x3002");
}

std::wstring RegistryStore::location(const Entry &entry) const {
    HKEY hive = root(entry.source);
    std::wstring name = hive == HKEY_CURRENT_USER ? L"HKEY_CURRENT_USER" : L"HKEY_LOCAL_MACHINE";
    return name + L"\\" + path(entry.source, entry.object, entry.verb);
}

void RegistryStore::exportEntry(const Entry &entry, const std::wstring &filename) const {
    checkCurrent(entry);
    std::wostringstream out;
    out << L"Windows Registry Editor Version 5.00\r\n\r\n";
    // Replace this single branch on import so added flags/subkeys are also undone.
    out << L"[-" << location(entry) << L"]\r\n\r\n";
    exportTree(out, location(entry), entry.snapshot);
    writeRegFile(filename, out.str(), true);
}

std::wstring RegistryStore::backup(const Entry &entry) const {
    makeDirectories(backupDirectory_);
    std::wostringstream contents;
    contents << L"Windows Registry Editor Version 5.00\r\n\r\n";
    contents << L"[-" << location(entry) << L"]\r\n\r\n";
    exportTree(contents, location(entry), entry.snapshot);
    SYSTEMTIME time; GetLocalTime(&time);
    wchar_t name[100];
    for (unsigned i = 0; i < 1000; ++i) {
        wsprintfW(name, L"\\menu-%04u%02u%02u-%02u%02u%02u-%03u-%lu-%u.reg",
            time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
            time.wMilliseconds, GetCurrentProcessId(), i);
        std::wstring file = backupDirectory_ + name;
        try { writeRegFile(file, contents.str(), false); return file; }
        catch (const Error &failure) { if (failure.code != ERROR_FILE_EXISTS && failure.code != ERROR_ALREADY_EXISTS) throw; }
    }
    throw Error(L"\x65e0\x6cd5\x751f\x6210\x552f\x4e00\x7684\x5907\x4efd\x6587\x4ef6\x540d\x3002");
}

bool sameTree(const Tree &a, const Tree &b) {
    if (a.name != b.name || a.values.size() != b.values.size() || a.children.size() != b.children.size()) return false;
    for (size_t i = 0; i < a.values.size(); ++i)
        if (a.values[i].name != b.values[i].name || a.values[i].type != b.values[i].type ||
            a.values[i].bytes != b.values[i].bytes) return false;
    for (size_t i = 0; i < a.children.size(); ++i)
        if (!sameTree(a.children[i], b.children[i])) return false;
    return true;
}

std::wstring trim(const std::wstring &text) {
    size_t start = text.find_first_not_of(L" \t\r\n"), end = text.find_last_not_of(L" \t\r\n");
    if (start == std::wstring::npos) return std::wstring();
    return text.substr(start, end - start + 1);
}

std::wstring buildCommand(const std::wstring &executable, const std::wstring &arguments) {
    std::wstring file = trim(executable), args = trim(arguments);
    if (file.empty() || file.find_first_of(L"\"\r\n") != std::wstring::npos)
        throw Error(L"\x8bf7\x9009\x62e9\x7a0b\x5e8f\x8def\x5f84\xff0c\x8def\x5f84\x5185\x5bb9\x4e0d\x8981\x5305\x542b\x5f15\x53f7\x6216\x6362\x884c\x3002");
    std::wstring result = L"\"" + file + L"\"";
    if (!args.empty()) result += L" " + args;
    return result;
}

bool splitCommand(const std::wstring &command, std::wstring &executable, std::wstring &arguments) {
    executable.clear(); arguments.clear();
    std::wstring text = trim(command);
    if (text.empty()) return false;
    size_t end;
    if (text[0] == L'"') {
        end = text.find(L'"', 1);
        if (end == std::wstring::npos || end == 1 ||
            (end + 1 < text.size() && text[end + 1] != L' ' && text[end + 1] != L'\t')) return false;
        executable = text.substr(1, end - 1);
        arguments = trim(text.substr(end + 1));
    } else {
        end = text.find_first_of(L" \t");
        executable = text.substr(0, end);
        std::wstring lower = executable;
        CharLowerBuffW(&lower[0], (DWORD)lower.size());
        if (lower.size() < 4) return false;
        std::wstring suffix = lower.substr(lower.size() - 4);
        if (suffix != L".exe" && suffix != L".com" && suffix != L".cmd" && suffix != L".bat") return false;
        if (end != std::wstring::npos) arguments = trim(text.substr(end));
    }
    return true;
}

bool containsInsensitive(const std::wstring &text, const std::wstring &needle) {
    if (needle.empty()) return true;
    std::wstring a = text, b = needle;
    if (!a.empty()) CharLowerBuffW(&a[0], (DWORD)a.size());
    CharLowerBuffW(&b[0], (DWORD)b.size());
    return a.find(b) != std::wstring::npos;
}

}
