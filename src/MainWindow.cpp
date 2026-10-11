#include "MainWindow.h"
#include "resource.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <vector>
#include <sstream>
#ifdef MIXP_TEST_BUILD
#include "../tests/WindowCapture.h"
#endif

namespace {
using namespace mixp;

class Window {
    HWND window_;
    RegistryStore &store_;
    std::vector<Entry> entries_;
    int selected_, scope_, sourceFilter_;
    bool loading_, dirty_, creating_;
    std::wstring object_;

    HWND control(int id) { return GetDlgItem(window_, id); }
    void text(int id, const std::wstring &s) { SetWindowTextW(control(id), s.c_str()); }
    std::wstring text(int id) {
        int size = GetWindowTextLengthW(control(id));
        std::vector<wchar_t> buffer(size + 1);
        GetWindowTextW(control(id), &buffer[0], size + 1);
        return &buffer[0];
    }
    void enabled(int id, bool yes) { EnableWindow(control(id), yes); }
    bool checked(int id) { return IsDlgButtonChecked(window_, id) == BST_CHECKED; }
    void checkBox(int id, bool yes) { CheckDlgButton(window_, id, yes ? BST_CHECKED : BST_UNCHECKED); }
    int combo(int id) { return (int)SendMessageW(control(id), CB_GETCURSEL, 0, 0); }
    void combo(int id, int index) { SendMessageW(control(id), CB_SETCURSEL, index, 0); }
    void addCombo(int id, const wchar_t *s) { SendMessageW(control(id), CB_ADDSTRING, 0, (LPARAM)s); }

    void error(const Error &e) {
        MessageBoxW(window_, e.message.c_str(), L"MixP Toolbox - \x64cd\x4f5c\x672a\x5b8c\x6210", MB_OK | MB_ICONERROR);
    }
    bool discard() {
        if (!dirty_) return true;
        return MessageBoxW(window_, L"\x7f16\x8f91\x5185\x5bb9\x5c1a\x672a\x4fdd\x5b58\x3002\x8981\x653e\x5f03\x8fd9\x4e9b\x4fee\x6539\x5417\xff1f", L"MixP Toolbox",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES;
    }
    void modes() {
        bool editable = creating_ || (selected_ >= 0 && entries_[selected_].editable);
        bool raw = checked(IDC_RAW);
        enabled(IDC_VERB, creating_);
        enabled(IDC_SOURCE, creating_);
        enabled(IDC_LABEL, editable);
        enabled(IDC_PROGRAM, editable && !raw);
        enabled(IDC_ARGUMENTS, editable && !raw);
        enabled(IDC_BROWSE, editable && !raw);
        enabled(IDC_RAW, editable);
        enabled(IDC_ENABLED, editable);
        enabled(IDC_EXTENDED, editable);
        enabled(IDC_ENVIRONMENT, editable);
        SendMessageW(control(IDC_COMMAND), EM_SETREADONLY, !editable || !raw, 0);
        enabled(IDC_SAVE, editable && dirty_ && !trim(text(IDC_COMMAND)).empty());
        enabled(IDC_DELETE, selected_ >= 0);
        enabled(IDC_EXPORT, selected_ >= 0);
    }
    void changed(bool command) {
        if (loading_) return;
        dirty_ = true;
        if (command && !checked(IDC_RAW)) {
            loading_ = true;
            try { text(IDC_COMMAND, buildCommand(text(IDC_PROGRAM), text(IDC_ARGUMENTS))); }
            catch (const Error &) { text(IDC_COMMAND, L""); }
            loading_ = false;
        }
        modes();
    }
    void newEditor(bool creating = false) {
        loading_ = true;
        selected_ = -1;
        creating_ = creating;
        std::wstring verb = L"MixpOpen";
        for (unsigned suffix = 2;; ++suffix) {
            bool exists = false;
            for (size_t i = 0; i < entries_.size(); ++i)
                if (entries_[i].object == object_ && !lstrcmpiW(entries_[i].verb.c_str(), verb.c_str())) exists = true;
            if (!exists) break;
            std::wostringstream next; next << L"MixpOpen" << suffix; verb = next.str();
        }
        if (!creating) verb.clear();
        text(IDC_VERB, verb); text(IDC_LABEL, L""); text(IDC_PROGRAM, L"");
        text(IDC_ARGUMENTS, creating ? L"\"%1\"" : L"");
        text(IDC_COMMAND, L"");
        combo(IDC_SOURCE, sourceFilter_ == AllUsers ? AllUsers : CurrentUser);
        checkBox(IDC_ENABLED, true); checkBox(IDC_EXTENDED, false);
        checkBox(IDC_ENVIRONMENT, false); checkBox(IDC_RAW, false);
        text(IDC_SAVE, creating ? L"\x6dfb\x52a0\x83dc\x5355" : L"\x4fdd\x5b58");
        text(IDC_LOCATION, creating ? L"\x65b0\x589e\x6a21\x5f0f\xff1a\x586b\x5199\x83dc\x5355\x540d\x79f0\x3001\x7a0b\x5e8f\x8def\x5f84\x548c\x53c2\x6570\xff0c\x518d\x70b9\x201c\x6dfb\x52a0\x83dc\x5355\x201d\x3002" :
            L"\x9009\x4e2d\x8868\x683c\x4e2d\x7684\x83dc\x5355\x8fdb\x884c\x7f16\x8f91\xff0c\x6216\x70b9\x51fb\x201c\x65b0\x589e\x201d\x6dfb\x52a0\x83dc\x5355\x3002");
        dirty_ = false; loading_ = false;
        modes();
    }
    void loadEditor(int index) {
        loading_ = true;
        selected_ = index;
        creating_ = false;
        text(IDC_SAVE, L"\x4fdd\x5b58");
        const Entry &e = entries_[index];
        text(IDC_VERB, e.verb); text(IDC_LABEL, e.label);
        combo(IDC_SOURCE, e.source);
        std::wstring program, arguments;
        bool simple = splitCommand(e.command, program, arguments);
        if (!simple) { program.clear(); arguments.clear(); }
        text(IDC_PROGRAM, program);
        text(IDC_ARGUMENTS, arguments);
        text(IDC_COMMAND, e.command);
        checkBox(IDC_RAW, !simple); checkBox(IDC_ENABLED, e.enabled);
        checkBox(IDC_EXTENDED, e.extended); checkBox(IDC_ENVIRONMENT, e.commandType == REG_EXPAND_SZ);
        text(IDC_LOCATION, store_.location(e));
        text(IDC_STATUS, e.editable ? L"\x7f16\x8f91\x540e\x70b9\x201c\x4fdd\x5b58\x201d\xff1b\x4fee\x6539\x524d\x4f1a\x81ea\x52a8\x5907\x4efd\x3002" :
            L"\x6b64\x9879\x7531 DDE\x3001\x6269\x5c55\x6216\x5b50\x83dc\x5355\x5904\x7406\xff0c\x53ef\x67e5\x770b\x3001\x5bfc\x51fa\x6216\x5220\x9664\xff0c\x4e0d\x80fd\x4f5c\x4e3a\x666e\x901a\x547d\x4ee4\x7f16\x8f91\x3002");
        dirty_ = false; loading_ = false;
        modes();
    }
    void column(int index, const wchar_t *name, int width) {
        LVCOLUMNW c; ZeroMemory(&c, sizeof(c));
        c.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        c.pszText = (LPWSTR)name; c.cx = width; c.iSubItem = index;
        SendMessageW(control(IDC_LIST), LVM_INSERTCOLUMNW, index, (LPARAM)&c);
    }
    void cell(int row, int col, const std::wstring &value) {
        LVITEMW item; ZeroMemory(&item, sizeof(item));
        item.iItem = row; item.iSubItem = col; item.pszText = (LPWSTR)value.c_str();
        SendMessageW(control(IDC_LIST), LVM_SETITEMTEXTW, row, (LPARAM)&item);
    }
    void showList() {
        bool wasLoading = loading_; loading_ = true;
        HWND list = control(IDC_LIST);
        SendMessageW(list, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(list);
        std::wstring query = trim(text(IDC_SEARCH));
        int row = 0;
        for (size_t i = 0; i < entries_.size(); ++i) {
            const Entry &e = entries_[i];
            if (!containsInsensitive(e.label, query) && !containsInsensitive(e.verb, query) &&
                !containsInsensitive(e.command, query) && !containsInsensitive(e.object, query)) continue;
            LVITEMW item; ZeroMemory(&item, sizeof(item));
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = row; item.lParam = (LPARAM)i;
            std::wstring label = e.label.empty() ? e.verb : e.label;
            item.pszText = (LPWSTR)label.c_str();
            SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
            cell(row, 1, e.verb);
            cell(row, 2, e.object);
            cell(row, 3, e.source == CurrentUser ? L"\x5f53\x524d\x7528\x6237" : L"\x6240\x6709\x7528\x6237");
            cell(row, 4, !e.enabled ? L"\x7981\x7528" : (e.extended ? L"Shift \x663e\x793a" : L"\x542f\x7528"));
            cell(row, 5, e.command);
            if ((int)i == selected_) ListView_SetItemState(list, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            ++row;
        }
        SendMessageW(list, WM_SETREDRAW, TRUE, 0); InvalidateRect(list, NULL, TRUE);
        text(IDC_EMPTY, entries_.empty() ?
            L"\x6b64\x8303\x56f4\x6682\x65e0\x9759\x6001\x83dc\x5355\x3002\r\n\x53ef\x5207\x6362\x8303\x56f4\x67e5\x770b\xff0c\x6216\x70b9\x201c\x65b0\x589e\x201d\x6dfb\x52a0\x3002" :
            L"\x6ca1\x6709\x5339\x914d\x7684\x83dc\x5355\x3002\r\n\x8bf7\x6e05\x7a7a\x641c\x7d22\x6761\x4ef6\x540e\x67e5\x770b\x3002");
        ShowWindow(control(IDC_EMPTY), row ? SW_HIDE : SW_SHOWNOACTIVATE);
        loading_ = wasLoading;
        std::wostringstream count;
        count << L"\x663e\x793a " << row << L" / " << entries_.size() << L" \x9879\xff1b\x9009\x4e2d\x9879\x53ef\x7f16\x8f91\xff0c\x70b9\x201c\x65b0\x589e\x201d\x6dfb\x52a0\x83dc\x5355\x3002";
        text(IDC_STATUS, count.str());
    }
    void refresh() {
        std::wstring object;
        std::vector<Entry> items;
        if (scope_ == AllScopes) items = store_.listAll(sourceFilter_);
        else {
            object = store_.objectFor((Scope)scope_, text(IDC_EXTENSION));
            items = store_.list(object, sourceFilter_);
        }
        object_ = object; entries_.swap(items);
        newEditor(); showList();
    }
    void labels() {
        SetWindowTextW(window_, L"MixP Toolbox 0.1.1 - \x53f3\x952e\x83dc\x5355\x7ba1\x7406");
        text(IDC_HEADER, L"MixP Toolbox 0.1.1  /  \x53f3\x952e\x83dc\x5355\x7ba1\x7406");
        text(IDC_LABEL_SCOPE, L"\x4f5c\x7528\x8303\x56f4"); text(IDC_LABEL_EXT, L"\x6269\x5c55\x540d");
        text(IDC_LABEL_VIEW, L"\x67e5\x770b\x6765\x6e90"); text(IDC_LABEL_SEARCH, L"\x641c\x7d22");
        text(IDC_REFRESH, L"\x5237\x65b0"); text(IDC_NEW, L"\x65b0\x589e"); text(IDC_DELETE, L"\x5220\x9664");
        text(IDC_EXPORT, L"\x5bfc\x51fa\x9009\x4e2d\x9879"); text(IDC_LABEL_VERB, L"\x5185\x90e8\x6807\x8bc6");
        text(IDC_LABEL_NAME, L"\x83dc\x5355\x540d\x79f0"); text(IDC_LABEL_SOURCE, L"\x4fdd\x5b58\x4f4d\x7f6e");
        text(IDC_LABEL_PROGRAM, L"\x7a0b\x5e8f\x8def\x5f84"); text(IDC_BROWSE, L"\x6d4f\x89c8\x2026");
        text(IDC_LABEL_ARGS, L"\x542f\x52a8\x53c2\x6570"); text(IDC_RAW, L"\x76f4\x63a5\x7f16\x8f91\x5b8c\x6574\x547d\x4ee4");
        text(IDC_LABEL_COMMAND, L"\x5b8c\x6574\x547d\x4ee4"); text(IDC_ENABLED, L"\x542f\x7528");
        text(IDC_EXTENDED, L"\x4ec5 Shift \x663e\x793a"); text(IDC_ENVIRONMENT, L"\x4f7f\x7528\x73af\x5883\x53d8\x91cf\xff08\x5982 %SystemRoot%\xff09");
        text(IDC_SAVE, L"\x4fdd\x5b58");
        text(IDC_HINT, L"\x53c2\x6570\x4e2d\x7684 \"%1\" \x8868\x793a\x6240\x9009\x6587\x4ef6\x3001\x6587\x4ef6\x5939\x6216\x9a71\x52a8\x5668\xff1b\x7a0b\x5e8f\x8def\x5f84\x4f1a\x81ea\x52a8\x52a0\x5f15\x53f7\x3002");
    }
    void browse() {
        wchar_t file[MAX_PATH] = {0};
        OPENFILENAMEW dialog; ZeroMemory(&dialog, sizeof(dialog));
        dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = window_;
        dialog.lpstrFilter = L"\x7a0b\x5e8f (*.exe;*.com;*.bat;*.cmd)\0*.exe;*.com;*.bat;*.cmd\0\x6240\x6709\x6587\x4ef6\0*.*\0\0";
        dialog.lpstrFile = file; dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        dialog.lpstrTitle = L"\x9009\x62e9\x53f3\x952e\x83dc\x5355\x8981\x542f\x52a8\x7684\x7a0b\x5e8f";
        if (GetOpenFileNameW(&dialog)) text(IDC_PROGRAM, file);
    }
public:
    explicit Window(RegistryStore &store) : window_(NULL), store_(store), selected_(-1),
        scope_(AllScopes), sourceFilter_(-1), loading_(true), dirty_(false), creating_(false) {}
    void initialize(HWND window) {
        window_ = window;
        labels();
        addCombo(IDC_SCOPE, L"\x6240\x6709\x6587\x4ef6"); addCombo(IDC_SCOPE, L"\x6587\x4ef6\x5939");
        addCombo(IDC_SCOPE, L"\x9a71\x52a8\x5668");
        addCombo(IDC_SCOPE, L"\x6587\x4ef6\x548c\x6587\x4ef6\x5939"); addCombo(IDC_SCOPE, L"\x6307\x5b9a\x6269\x5c55\x540d");
        addCombo(IDC_SCOPE, L"\x5168\x90e8\x8303\x56f4"); combo(IDC_SCOPE, AllScopes);
        addCombo(IDC_VIEW_SOURCE, L"\x5168\x90e8\xff08\x5206\x6765\x6e90\xff09"); addCombo(IDC_VIEW_SOURCE, L"\x5f53\x524d\x7528\x6237");
        addCombo(IDC_VIEW_SOURCE, L"\x6240\x6709\x7528\x6237"); combo(IDC_VIEW_SOURCE, 0);
        addCombo(IDC_SOURCE, L"\x5f53\x524d\x7528\x6237 (HKCU)"); addCombo(IDC_SOURCE, L"\x6240\x6709\x7528\x6237 (HKLM)");
        text(IDC_EXTENSION, L".txt"); enabled(IDC_EXTENSION, false);
        SendMessageW(control(IDC_VERB), EM_SETLIMITTEXT, 255, 0);
        SendMessageW(control(IDC_EXTENSION), EM_SETLIMITTEXT, 255, 0);
        SendMessageW(control(IDC_COMMAND), EM_SETLIMITTEXT, 32767, 0);
        SendMessageW(control(IDC_ARGUMENTS), EM_SETLIMITTEXT, 32767, 0);
        ListView_SetExtendedListViewStyle(control(IDC_LIST), LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        SendMessageW(control(IDC_LIST), LVM_SETUNICODEFORMAT, TRUE, 0);
        column(0, L"\x83dc\x5355\x540d\x79f0", 160); column(1, L"\x5185\x90e8\x6807\x8bc6", 120);
        column(2, L"\x4f5c\x7528\x7c7b\x578b", 130); column(3, L"\x6765\x6e90", 90);
        column(4, L"\x72b6\x6001", 80); column(5, L"\x542f\x52a8\x547d\x4ee4", 350);
        loading_ = false;
        refresh();
        HICON icon = LoadIcon(NULL, IDI_APPLICATION);
        SendMessageW(window_, WM_SETICON, ICON_BIG, (LPARAM)icon);
        SendMessageW(window_, WM_SETICON, ICON_SMALL, (LPARAM)icon);
    }
    INT_PTR message(UINT message, WPARAM wparam, LPARAM lparam);
};

}

INT_PTR Window::message(UINT message, WPARAM wparam, LPARAM lparam) {
#ifdef MIXP_TEST_BUILD
    if (message == WM_APP + 101) {
        SetWindowLongW(window_, DWL_MSGRESULT, captureTestWindow(window_));
        return TRUE;
    }
#endif
    if (message == WM_CLOSE) {
        if (discard()) DestroyWindow(window_);
        return TRUE;
    }
    if (message == WM_DESTROY) { PostQuitMessage(0); return TRUE; }
    if (message == WM_NOTIFY) {
        NMHDR *header = (NMHDR *)lparam;
        if (header->idFrom == IDC_LIST && header->code == LVN_ITEMCHANGED && !loading_) {
            NMLISTVIEW *change = (NMLISTVIEW *)lparam;
            if ((change->uChanged & LVIF_STATE) && (change->uNewState & LVIS_SELECTED) &&
                !(change->uOldState & LVIS_SELECTED)) {
                LVITEMW item; ZeroMemory(&item, sizeof(item));
                item.mask = LVIF_PARAM; item.iItem = change->iItem;
                SendMessageW(control(IDC_LIST), LVM_GETITEMW, 0, (LPARAM)&item);
                int index = (int)item.lParam;
                if (index != selected_ && index >= 0 && index < (int)entries_.size()) {
                    if (discard()) loadEditor(index);
                    else showList();
                }
            }
        }
        return FALSE;
    }
    if (message != WM_COMMAND || loading_) return FALSE;
    int id = LOWORD(wparam), event = HIWORD(wparam);
    if (id == IDCANCEL) { SendMessageW(window_, WM_CLOSE, 0, 0); return TRUE; }
    if (event == EN_CHANGE) {
        if (id == IDC_EXTENSION) text(IDC_STATUS, L"\x6269\x5c55\x540d\x5df2\x66f4\x6539\xff0c\x70b9\x51fb\x201c\x5237\x65b0\x201d\x67e5\x8be2\x8be5\x6587\x4ef6\x7c7b\x578b\x3002");
        else if (id == IDC_SEARCH) showList();
        else if (id == IDC_PROGRAM || id == IDC_ARGUMENTS) changed(true);
        else if (id == IDC_LABEL || id == IDC_VERB || id == IDC_COMMAND) changed(false);
        return TRUE;
    }
    if (event == CBN_SELCHANGE && (id == IDC_SCOPE || id == IDC_VIEW_SOURCE)) {
        int newScope = combo(IDC_SCOPE), newFilter = combo(IDC_VIEW_SOURCE) - 1;
        if (!discard()) {
            combo(IDC_SCOPE, scope_); combo(IDC_VIEW_SOURCE, sourceFilter_ + 1);
            return TRUE;
        }
        scope_ = newScope; sourceFilter_ = newFilter;
        enabled(IDC_EXTENSION, scope_ == Extension);
        try { refresh(); }
        catch (...) { entries_.clear(); object_.clear(); newEditor(); showList(); throw; }
        return TRUE;
    }
    if (event == CBN_SELCHANGE && id == IDC_SOURCE) { changed(false); return TRUE; }
    if (event != BN_CLICKED) return FALSE;
    switch (id) {
    case IDC_REFRESH:
        if (discard()) refresh();
        break;
    case IDC_NEW:
        if (discard()) {
            // New items need one concrete target; an overview defaults to all files.
            if (scope_ == AllScopes) {
                scope_ = AllFiles; combo(IDC_SCOPE, scope_); refresh();
            }
            // Validate the type before offering an editor for a new menu item.
            object_ = store_.objectFor((Scope)scope_, text(IDC_EXTENSION));
            newEditor(true); showList(); SetFocus(control(IDC_LABEL));
            text(IDC_STATUS, L"\x5df2\x8fdb\x5165\x65b0\x589e\x6a21\x5f0f\xff0c\x8bf7\x586b\x5199\x4e0b\x65b9\x7684\x83dc\x5355\x540d\x79f0\x548c\x7a0b\x5e8f\x8def\x5f84\x3002");
        }
        break;
    case IDC_BROWSE: browse(); break;
    case IDC_RAW:
        if (!checked(IDC_RAW)) {
            std::wstring program, arguments;
            if (!splitCommand(text(IDC_COMMAND), program, arguments)) {
                checkBox(IDC_RAW, true);
                throw Error(L"\x6b64\x547d\x4ee4\x65e0\x6cd5\x62c6\x5206\x4e3a\x7a0b\x5e8f\x548c\x53c2\x6570\xff0c\x8bf7\x7ee7\x7eed\x4f7f\x7528\x5b8c\x6574\x547d\x4ee4\x7f16\x8f91\x3002");
            }
            loading_ = true;
            text(IDC_PROGRAM, program); text(IDC_ARGUMENTS, arguments);
            loading_ = false;
        }
        changed(false); break;
    case IDC_ENABLED: case IDC_EXTENDED: case IDC_ENVIRONMENT:
        changed(false); break;
    case IDC_SAVE: {
        Draft draft;
        draft.source = (Source)combo(IDC_SOURCE);
        draft.object = selected_ < 0 ? store_.objectFor((Scope)scope_, text(IDC_EXTENSION)) : entries_[selected_].object;
        draft.verb = trim(text(IDC_VERB)); draft.label = text(IDC_LABEL);
        draft.command = text(IDC_COMMAND);
        draft.enabled = checked(IDC_ENABLED); draft.extended = checked(IDC_EXTENDED);
        draft.expandEnvironment = checked(IDC_ENVIRONMENT);
        std::wstring saved;
        if (selected_ >= 0) saved = store_.update(entries_[selected_], draft);
        else store_.create(draft);
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
        dirty_ = false;
        // A successful save must be visible even when the old filter excluded it.
        loading_ = true;
        text(IDC_SEARCH, L"");
        if (sourceFilter_ >= 0 && sourceFilter_ != draft.source) {
            sourceFilter_ = -1; combo(IDC_VIEW_SOURCE, 0);
        }
        loading_ = false;
        refresh();
        for (size_t i = 0; i < entries_.size(); ++i)
            if (entries_[i].source == draft.source && entries_[i].object == draft.object &&
                !lstrcmpiW(entries_[i].verb.c_str(), draft.verb.c_str())) {
                loadEditor((int)i); showList(); break;
            }
        if (saved.empty()) text(IDC_STATUS, L"\x5df2\x65b0\x589e\x83dc\x5355\x9879\x3002\x8bf7\x5728\x8d44\x6e90\x7ba1\x7406\x5668\x4e2d\x91cd\x65b0\x6253\x5f00\x53f3\x952e\x83dc\x5355\x67e5\x770b\x3002");
        else text(IDC_STATUS, L"\x5df2\x4fdd\x5b58\xff1b\x4fee\x6539\x524d\x5907\x4efd\xff1a" + saved);
        break;
    }
    case IDC_DELETE:
        if (selected_ >= 0) {
            const Entry &entry = entries_[selected_];
            std::wstring prompt = L"\x5220\x9664\x8fd9\x4e2a\x83dc\x5355\x9879\x53ca\x5176\x5b50\x9879\xff1f\r\n\r\n" +
                (entry.label.empty() ? entry.verb : entry.label) + L"\r\n" + store_.location(entry) +
                L"\r\n\r\n\x5220\x9664\x524d\x4f1a\x81ea\x52a8\x5907\x4efd\xff0c\x53ef\x901a\x8fc7\x5907\x4efd\x6062\x590d\x3002";
            if (MessageBoxW(window_, prompt.c_str(), L"\x5220\x9664\x53f3\x952e\x83dc\x5355\x9879", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                std::wstring saved = store_.remove(entry);
                SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
                dirty_ = false; refresh();
                text(IDC_STATUS, L"\x5df2\x5220\x9664\xff1b\x5907\x4efd\xff1a" + saved);
            }
        }
        break;
    case IDC_EXPORT:
        if (selected_ >= 0) {
            wchar_t filename[MAX_PATH] = L"menu-backup.reg";
            OPENFILENAMEW dialog; ZeroMemory(&dialog, sizeof(dialog));
            dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = window_;
            dialog.lpstrFilter = L"\x6ce8\x518c\x8868\x5907\x4efd (*.reg)\0*.reg\0\x6240\x6709\x6587\x4ef6\0*.*\0\0";
            dialog.lpstrFile = filename; dialog.nMaxFile = MAX_PATH;
            dialog.lpstrDefExt = L"reg";
            dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetSaveFileNameW(&dialog)) {
                store_.exportEntry(entries_[selected_], filename);
                text(IDC_STATUS, L"\x5df2\x5bfc\x51fa\xff1a" + std::wstring(filename));
            }
        }
        break;
    default: return FALSE;
    }
    return TRUE;
}

static INT_PTR CALLBACK dialogProc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam) {
    Window *window = (Window *)GetWindowLongW(dialog, GWL_USERDATA);
    try {
        if (message == WM_INITDIALOG) {
            window = (Window *)lparam;
            SetWindowLongW(dialog, GWL_USERDATA, (LONG)window);
            window->initialize(dialog);
            return TRUE;
        }
        if (window) return window->message(message, wparam, lparam);
    } catch (const mixp::Error &failure) {
        MessageBoxW(dialog, failure.message.c_str(), L"MixP Toolbox - \x64cd\x4f5c\x672a\x5b8c\x6210", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxW(dialog, L"\x64cd\x4f5c\x672a\x5b8c\x6210\xff0c\x8bf7\x5237\x65b0\x540e\x91cd\x8bd5\x3002", L"MixP Toolbox", MB_OK | MB_ICONERROR);
    }
    return FALSE;
}

int runMainWindow(HINSTANCE instance, mixp::RegistryStore &store, int show) {
    INITCOMMONCONTROLSEX controls; controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES;
    if (!InitCommonControlsEx(&controls)) throw mixp::Error(L"\x65e0\x6cd5\x521d\x59cb\x5316\x5217\x8868\x63a7\x4ef6\x3002");
    Window window(store);
    HWND dialog = CreateDialogParamW(instance, MAKEINTRESOURCEW(IDD_MAIN), NULL, dialogProc, (LPARAM)&window);
    if (!dialog) throw mixp::Error(L"\x65e0\x6cd5\x521b\x5efa\x7a0b\x5e8f\x7a97\x53e3\x3002", GetLastError());
    ShowWindow(dialog, show); UpdateWindow(dialog);
    MSG message;
    int result;
    while ((result = GetMessageW(&message, NULL, 0, 0)) > 0) {
        if (!IsDialogMessageW(dialog, &message)) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
    }
    return result < 0 ? 1 : (int)message.wParam;
}
