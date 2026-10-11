#ifndef MIXP_TEST_WINDOW_CAPTURE_H
#define MIXP_TEST_WINDOW_CAPTURE_H
// Called in the GUI process: Wine device contexts cannot be shared with another process.
static BOOL captureTestWindow(HWND window) {
    RECT rect; GetWindowRect(window, &rect);
    int width = rect.right - rect.left, height = rect.bottom - rect.top;
    BITMAPINFO info; ZeroMemory(&info, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = height; info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 24;
    DWORD stride = (width * 3 + 3) & ~3, size = stride * height;
    HDC screen = GetWindowDC(window), memory = CreateCompatibleDC(screen);
    void *pixels = NULL; HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (!screen || !memory || !bitmap) return FALSE;
    HGDIOBJ old = SelectObject(memory, bitmap);
    BitBlt(memory, 0, 0, width, height, screen, 0, 0, SRCCOPY);
    GdiFlush();
    HANDLE file = CreateFileW(L"build\\Release\\gui-test.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    // The BMP disk header is 14 bytes, even when BCB6 pads BITMAPFILEHEADER to 16.
    BYTE header[14] = {0x42, 0x4d}; DWORD total = 14 + sizeof(BITMAPINFOHEADER) + size;
    DWORD offset = 14 + sizeof(BITMAPINFOHEADER);
    for (unsigned i = 0; i < 4; ++i) { header[2+i] = (BYTE)(total >> (i * 8)); header[10+i] = (BYTE)(offset >> (i * 8)); }
    DWORD written = 0;
    BOOL ok = file != INVALID_HANDLE_VALUE;
    if (ok) {
        ok = WriteFile(file, header, sizeof(header), &written, NULL) && written == sizeof(header);
        ok = ok && WriteFile(file, &info.bmiHeader, sizeof(BITMAPINFOHEADER), &written, NULL) && written == sizeof(BITMAPINFOHEADER);
        ok = ok && WriteFile(file, pixels, size, &written, NULL) && written == size;
        CloseHandle(file);
    }
    SelectObject(memory, old); DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(window, screen);
    return ok;
}
#endif
