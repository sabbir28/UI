#include <windows.h>
#include "lib/UI/include/Toggle.h"

UI::Toggle g_toggle;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            g_toggle.Create(hwnd, 50, 50, 120, 50, 1001,3);
            g_toggle.SetSwitchStyle(2);  // 0–5
            g_toggle.SetBodyStyle(2);    // 0–9
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1001)
            {
                MessageBoxW(hwnd,
                            g_toggle.IsChecked() ? L"Toggle ON" : L"Toggle OFF",
                            L"State",
                            MB_OK);
            }
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    UI::Toggle::Register(hInstance);

    const wchar_t CLASS_NAME[] = L"MainWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
            0,
            CLASS_NAME,
            L"Toggle Example",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            500, 400,
            nullptr,
            nullptr,
            hInstance,
            nullptr
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}