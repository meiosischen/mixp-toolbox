#include <windows.h>
#include <shellapi.h>
#include "src/MainWindow.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    try {
#ifdef MIXP_TEST_BUILD
        wchar_t font[MAX_PATH];
        if (GetEnvironmentVariableW(L"MIXP_TEST_FONT", font, MAX_PATH)) AddFontResourceW(font);
        int count = 0;
        LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &count);
        if (!args || count != 2 || wcsncmp(args[1], L"Software\\MixpToolboxTests\\", 26)) return 2;
        std::wstring base = args[1];
        LocalFree(args);
        wchar_t temp[MAX_PATH];
        GetTempPathW(MAX_PATH, temp);
        mixp::RegistryStore store(HKEY_CURRENT_USER, base + L"\\User\\Classes",
            HKEY_CURRENT_USER, base + L"\\Machine\\Classes", std::wstring(temp) + L"MixpToolboxGuiBackups");
#else
        mixp::RegistryStore store;
#endif
        return runMainWindow(instance, store, show);
    } catch (const mixp::Error &error) {
        MessageBoxW(NULL, error.message.c_str(), L"MixP Toolbox", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxW(NULL, L"\x65e0\x6cd5\x521d\x59cb\x5316 MixP Toolbox\x3002", L"MixP Toolbox", MB_OK | MB_ICONERROR);
    }
    return 1;
}
