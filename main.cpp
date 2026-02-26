#include <windows.h>

#include <string>

#include "lib/UI/include/Toggle.h"

typedef BOOL(*RegisterClassFn)(HINSTANCE);
typedef UIToggleHandle(*CreateFn)(const UIToggleCreateParams*);
typedef void(*DestroyFn)(UIToggleHandle);
typedef BOOL(*SetCheckedFn)(UIToggleHandle, BOOL, BOOL);
typedef BOOL(*GetCheckedFn)(UIToggleHandle, BOOL*);
typedef BOOL(*SetStyleFn)(UIToggleHandle, int);

struct ToggleApi
{
    HMODULE module = nullptr;
    RegisterClassFn registerClass = nullptr;
    CreateFn create = nullptr;
    DestroyFn destroy = nullptr;
    SetCheckedFn setChecked = nullptr;
    GetCheckedFn getChecked = nullptr;
    SetStyleFn setSwitchStyle = nullptr;
    SetStyleFn setBodyStyle = nullptr;
};

ToggleApi g_api;
UIToggleHandle g_toggle = nullptr;

bool LoadToggleApi(ToggleApi* api)
{
    api->module = LoadLibraryW(L"UIToggle.dll");
    if (api->module == nullptr)
    {
        return false;
    }

    api->registerClass = reinterpret_cast<RegisterClassFn>(GetProcAddress(api->module, "UIToggle_RegisterClass"));
    api->create = reinterpret_cast<CreateFn>(GetProcAddress(api->module, "UIToggle_Create"));
    api->destroy = reinterpret_cast<DestroyFn>(GetProcAddress(api->module, "UIToggle_Destroy"));
    api->setChecked = reinterpret_cast<SetCheckedFn>(GetProcAddress(api->module, "UIToggle_SetChecked"));
    api->getChecked = reinterpret_cast<GetCheckedFn>(GetProcAddress(api->module, "UIToggle_GetChecked"));
    api->setSwitchStyle = reinterpret_cast<SetStyleFn>(GetProcAddress(api->module, "UIToggle_SetSwitchStyle"));
    api->setBodyStyle = reinterpret_cast<SetStyleFn>(GetProcAddress(api->module, "UIToggle_SetBodyStyle"));

    return api->registerClass && api->create && api->destroy && api->setChecked && api->getChecked && api->setSwitchStyle && api->setBodyStyle;
}

void UnloadToggleApi(ToggleApi* api)
{
    if (api->module != nullptr)
    {
        FreeLibrary(api->module);
    }
    *api = {};
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            UIToggleCreateParams params{};
            params.parent = hwnd;
            params.x = 50;
            params.y = 50;
            params.width = 120;
            params.height = 50;
            params.control_id = 1001;
            params.radio_group = -1;

            g_toggle = g_api.create(&params);
            if (g_toggle != nullptr)
            {
                g_api.setSwitchStyle(g_toggle, 2);
                g_api.setBodyStyle(g_toggle, 2);
            }
            return 0;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == 1001 && g_toggle != nullptr)
            {
                BOOL checked = FALSE;
                if (g_api.getChecked(g_toggle, &checked))
                {
                    MessageBoxW(hwnd, checked ? L"Toggle ON" : L"Toggle OFF", L"State", MB_OK);
                }
            }
            return 0;

        case WM_DESTROY:
            if (g_toggle != nullptr)
            {
                g_api.destroy(g_toggle);
                g_toggle = nullptr;
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show)
{
    if (!LoadToggleApi(&g_api))
    {
        MessageBoxW(nullptr, L"Failed to load UIToggle.dll", L"Error", MB_ICONERROR);
        return 1;
    }

    if (!g_api.registerClass(instance))
    {
        MessageBoxW(nullptr, L"Failed to register toggle class", L"Error", MB_ICONERROR);
        UnloadToggleApi(&g_api);
        return 1;
    }

    const wchar_t className[] = L"ToggleSampleWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;

    RegisterClassW(&wc);

    HWND window = CreateWindowExW(
        0,
        className,
        L"Toggle DLL Sample",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        300,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (window == nullptr)
    {
        UnloadToggleApi(&g_api);
        return 1;
    }

    ShowWindow(window, show);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnloadToggleApi(&g_api);
    return 0;
}
