#ifndef MIXP_MENU_REGISTRY_H
#define MIXP_MENU_REGISTRY_H

#include <windows.h>
#include <string>
#include <vector>

namespace mixp {

struct Error {
    std::wstring message;
    DWORD code;
    Error(const std::wstring &text, DWORD status = 0);
};

enum Scope { AllFiles, Directories, Drives, FileSystemObjects, Extension };
enum Source { CurrentUser, AllUsers };

struct Value {
    std::wstring name;
    DWORD type;
    std::vector<unsigned char> bytes;
};

struct Tree {
    std::wstring name;
    std::vector<Value> values;
    std::vector<Tree> children;
};

struct Entry {
    Source source;
    std::wstring object;
    std::wstring verb;
    std::wstring label;
    std::wstring command;
    DWORD labelType;
    DWORD commandType;
    bool enabled;
    bool extended;
    bool editable;
    Tree snapshot;
    Entry();
};

struct Draft {
    Source source;
    std::wstring object;
    std::wstring verb;
    std::wstring label;
    std::wstring command;
    bool enabled;
    bool extended;
    bool expandEnvironment;
    Draft();
};

class RegistryStore {
public:
    RegistryStore();
    RegistryStore(HKEY userRoot, const std::wstring &userPrefix,
                  HKEY machineRoot, const std::wstring &machinePrefix,
                  const std::wstring &backupDirectory);
    std::wstring objectFor(Scope scope, const std::wstring &extension) const;
    std::vector<Entry> list(const std::wstring &object, int sourceFilter) const;
    Entry read(Source source, const std::wstring &object, const std::wstring &verb) const;
    void create(const Draft &draft);
    std::wstring update(const Entry &original, const Draft &draft);
    std::wstring remove(const Entry &original);
    void exportEntry(const Entry &entry, const std::wstring &filename) const;
    std::wstring location(const Entry &entry) const;
    static void deleteTree(HKEY root, const std::wstring &path);
private:
    HKEY userRoot_, machineRoot_;
    std::wstring userPrefix_, machinePrefix_, backupDirectory_;
    HKEY root(Source source) const;
    std::wstring prefix(Source source) const;
    std::wstring path(Source source, const std::wstring &object, const std::wstring &verb) const;
    std::wstring backup(const Entry &entry) const;
    void checkCurrent(const Entry &entry) const;
};

std::wstring buildCommand(const std::wstring &executable, const std::wstring &arguments);
bool splitCommand(const std::wstring &command, std::wstring &executable, std::wstring &arguments);
bool containsInsensitive(const std::wstring &text, const std::wstring &needle);
std::wstring trim(const std::wstring &text);
bool sameTree(const Tree &a, const Tree &b);

}
#endif
