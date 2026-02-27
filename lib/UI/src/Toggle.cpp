#include "../include/Toggle.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "msimg32.lib")

namespace
{
constexpr wchar_t kToggleClassName[] = L"UI_TOGGLE_CONTROL";
constexpr UINT_PTR kAnimationTimerId = 1;
constexpr int kAnimationStepPixels = 4;
constexpr std::uint32_t kHandleMagic = 0x54474C45; // TGLE

struct TileRect
{
    int x;
    int y;
    int width;
    int height;
};

struct ImageAtlas
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
    std::vector<TileRect> tiles;
};

struct ToggleControl;

struct UIToggleHandleTag
{
    std::uint32_t magic = kHandleMagic;
    ToggleControl* control = nullptr;
};

ImageAtlas g_bodyAtlas;
ImageAtlas g_switchAtlas;
HINSTANCE g_moduleInstance = nullptr;

std::wstring GetModuleDirectory(HINSTANCE instance)
{
    wchar_t path[MAX_PATH] = {};
    const DWORD chars = GetModuleFileNameW(instance, path, MAX_PATH);
    if (chars == 0 || chars == MAX_PATH)
    {
        return L".";
    }

    std::wstring fullPath(path, chars);
    const std::size_t pos = fullPath.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return L".";
    }

    return fullPath.substr(0, pos);
}

// Resolves the runtime assets directory relative to the DLL location.
std::wstring GetAssetsDirectory()
{
    return GetModuleDirectory(g_moduleInstance) + L"\\assets\\Troggle";
}

std::string WideToUtf8(const std::wstring& input)
{
    if (input.empty())
    {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0)
    {
        return {};
    }

    std::string output(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()), &output[0], required, nullptr, nullptr);
    return output;
}

int Clamp(int value, int minimum, int maximum)
{
    return std::max(minimum, std::min(maximum, value));
}

RECT ComputeVisibleBounds(const ImageAtlas& atlas, int index)
{
    RECT fallback{0, 0, 0, 0};
    if (index < 0 || index >= static_cast<int>(atlas.tiles.size()))
    {
        return fallback;
    }

    const TileRect& tile = atlas.tiles[static_cast<std::size_t>(index)];

    int minX = tile.width;
    int minY = tile.height;
    int maxX = 0;
    int maxY = 0;
    bool hasVisiblePixel = false;

    for (int y = 0; y < tile.height; ++y)
    {
        for (int x = 0; x < tile.width; ++x)
        {
            const int pxIndex = ((tile.y + y) * atlas.width + (tile.x + x)) * 4;
            if (atlas.pixels[static_cast<std::size_t>(pxIndex + 3)] > 0)
            {
                hasVisiblePixel = true;
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
    }

    if (!hasVisiblePixel)
    {
        return RECT{tile.x, tile.y, tile.x + tile.width, tile.y + tile.height};
    }

    return RECT{tile.x + minX, tile.y + minY, tile.x + maxX + 1, tile.y + maxY + 1};
}

ImageAtlas LoadAtlas(const std::wstring& path, int columns, int rows)
{
    ImageAtlas atlas;
    const std::string utf8Path = WideToUtf8(path);

    int channels = 0;
    unsigned char* rawData = stbi_load(utf8Path.c_str(), &atlas.width, &atlas.height, &channels, 4);
    if (rawData == nullptr)
    {
        throw std::runtime_error("Failed to load atlas image");
    }

    atlas.pixels.assign(rawData, rawData + (atlas.width * atlas.height * 4));
    stbi_image_free(rawData);

    for (int i = 0; i < atlas.width * atlas.height; ++i)
    {
        unsigned char* pixel = &atlas.pixels[static_cast<std::size_t>(i * 4)];
        const float alpha = static_cast<float>(pixel[3]) / 255.0f;
        pixel[0] = static_cast<unsigned char>(pixel[0] * alpha);
        pixel[1] = static_cast<unsigned char>(pixel[1] * alpha);
        pixel[2] = static_cast<unsigned char>(pixel[2] * alpha);
    }

    const int tileWidth = atlas.width / columns;
    const int tileHeight = atlas.height / rows;

    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            atlas.tiles.push_back(TileRect{col * tileWidth, row * tileHeight, tileWidth, tileHeight});
        }
    }

    return atlas;
}

// Lazy-load texture atlases once and keep them in memory for control instances.
bool EnsureAtlasesLoaded()
{
    if (!g_bodyAtlas.pixels.empty() && !g_switchAtlas.pixels.empty())
    {
        return true;
    }

    try
    {
        const std::wstring assetsDir = GetAssetsDirectory();
        g_bodyAtlas = LoadAtlas(assetsDir + L"\\switch-body.png", 5, 2);
        g_switchAtlas = LoadAtlas(assetsDir + L"\\Switch.png", 3, 2);
        return true;
    }
    catch (...)
    {
        g_bodyAtlas = {};
        g_switchAtlas = {};
        return false;
    }
}

// Draws one atlas tile with alpha blending into the destination rectangle.
void DrawTile(HDC hdc, const RECT& destination, const ImageAtlas& atlas, int tileIndex)
{
    if (tileIndex < 0 || tileIndex >= static_cast<int>(atlas.tiles.size()))
    {
        return;
    }

    const RECT crop = ComputeVisibleBounds(atlas, tileIndex);
    const int srcWidth = crop.right - crop.left;
    const int srcHeight = crop.bottom - crop.top;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = atlas.width;
    bmi.bmiHeader.biHeight = -atlas.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (bitmap == nullptr)
    {
        return;
    }

    std::memcpy(bits, atlas.pixels.data(), atlas.pixels.size());

    HDC memoryDc = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memoryDc, bitmap));

    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(
        hdc,
        destination.left,
        destination.top,
        destination.right - destination.left,
        destination.bottom - destination.top,
        memoryDc,
        crop.left,
        crop.top,
        srcWidth,
        srcHeight,
        blend);

    SelectObject(memoryDc, oldBitmap);
    DeleteDC(memoryDc);
    DeleteObject(bitmap);
}

struct ToggleControl
{
    HWND window = nullptr;
    UIToggleState state = UI_TOGGLE_STATE_OFF;
    int knobOffset = 0;
    int targetOffset = 0;
    int switchStyle = 0;
    int bodyStyle = 0;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        ToggleControl* self = nullptr;

        if (message == WM_NCCREATE)
        {
            CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<ToggleControl*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            self->window = hwnd;
        }

        self = reinterpret_cast<ToggleControl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (self == nullptr)
        {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }

        switch (message)
        {
            case WM_LBUTTONDOWN:
                self->SetChecked(self->state == UI_TOGGLE_STATE_OFF, TRUE);
                return 0;
            case WM_TIMER:
                if (wParam == kAnimationTimerId)
                {
                    self->AnimateStep();
                }
                return 0;
            case WM_PAINT:
                self->OnPaint();
                return 0;
            case WM_NCDESTROY:
                KillTimer(hwnd, kAnimationTimerId);
                self->window = nullptr;
                return DefWindowProcW(hwnd, message, wParam, lParam);
            default:
                return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    // Updates state, starts animation, and optionally notifies the parent window.
    bool SetChecked(BOOL checked, BOOL notifyParent)
    {
        if (!EnsureAtlasesLoaded() || window == nullptr)
        {
            return false;
        }

        state = checked ? UI_TOGGLE_STATE_ON : UI_TOGGLE_STATE_OFF;
        const int travel = g_switchAtlas.tiles.empty() ? 0 : g_switchAtlas.tiles[0].width;
        targetOffset = checked ? travel : 0;
        SetTimer(window, kAnimationTimerId, 10, nullptr);

        if (notifyParent)
        {
            SendMessageW(GetParent(window), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(window), BN_CLICKED), reinterpret_cast<LPARAM>(window));
        }

        return true;
    }

    void AnimateStep()
    {
        if (knobOffset < targetOffset)
        {
            knobOffset = std::min(knobOffset + kAnimationStepPixels, targetOffset);
        }
        else if (knobOffset > targetOffset)
        {
            knobOffset = std::max(knobOffset - kAnimationStepPixels, targetOffset);
        }
        else
        {
            KillTimer(window, kAnimationTimerId);
        }

        InvalidateRect(window, nullptr, TRUE);
    }

    void OnPaint()
    {
        PAINTSTRUCT paint{};
        HDC hdc = BeginPaint(window, &paint);

        RECT bounds{};
        GetClientRect(window, &bounds);

        DrawTile(hdc, bounds, g_bodyAtlas, bodyStyle);

        RECT knob = bounds;
        knob.left += knobOffset;
        knob.right += knobOffset;
        DrawTile(hdc, knob, g_switchAtlas, switchStyle);

        EndPaint(window, &paint);
    }
};

bool IsValidHandle(UIToggleHandle handle)
{
    return handle != nullptr && handle->magic == kHandleMagic && handle->control != nullptr;
}

} // namespace

BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_moduleInstance = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" BOOL UIToggle_RegisterClass(HINSTANCE instance)
{
    HINSTANCE classInstance = instance != nullptr ? instance : g_moduleInstance;

    WNDCLASSW wc{};
    wc.lpfnWndProc = ToggleControl::WindowProc;
    wc.hInstance = classInstance;
    wc.lpszClassName = kToggleClassName;
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);

    const ATOM result = RegisterClassW(&wc);
    if (result != 0)
    {
        return TRUE;
    }

    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

extern "C" UIToggleHandle UIToggle_Create(const UIToggleCreateParams* params)
{
    if (params == nullptr || params->parent == nullptr)
    {
        return nullptr;
    }

    if (!EnsureAtlasesLoaded())
    {
        return nullptr;
    }

    ToggleControl* control = new ToggleControl();
    UIToggleHandle handle = new UIToggleHandleTag();
    handle->control = control;

    HWND hwnd = CreateWindowExW(
        0,
        kToggleClassName,
        L"",
        WS_CHILD | WS_VISIBLE,
        params->x,
        params->y,
        params->width,
        params->height,
        params->parent,
        reinterpret_cast<HMENU>(static_cast<intptr_t>(params->control_id)),
        g_moduleInstance,
        control);

    if (hwnd == nullptr)
    {
        delete handle;
        delete control;
        return nullptr;
    }

    return handle;
}

extern "C" void UIToggle_Destroy(UIToggleHandle handle)
{
    if (!IsValidHandle(handle))
    {
        return;
    }

    ToggleControl* control = handle->control;
    handle->magic = 0;
    handle->control = nullptr;

    if (control->window != nullptr)
    {
        DestroyWindow(control->window);
    }

    delete control;
    delete handle;
}

extern "C" BOOL UIToggle_SetChecked(UIToggleHandle handle, BOOL checked, BOOL notify_parent)
{
    if (!IsValidHandle(handle))
    {
        return FALSE;
    }

    return handle->control->SetChecked(checked, notify_parent) ? TRUE : FALSE;
}

extern "C" BOOL UIToggle_GetChecked(UIToggleHandle handle, BOOL* checked)
{
    if (!IsValidHandle(handle) || checked == nullptr)
    {
        return FALSE;
    }

    *checked = (handle->control->state == UI_TOGGLE_STATE_ON) ? TRUE : FALSE;
    return TRUE;
}

// Applies a clamped switch-knob style index from the loaded switch atlas.
extern "C" BOOL UIToggle_SetSwitchStyle(UIToggleHandle handle, int style_index)
{
    if (!IsValidHandle(handle) || !EnsureAtlasesLoaded())
    {
        return FALSE;
    }

    ToggleControl* control = handle->control;
    const int maxStyle = static_cast<int>(g_switchAtlas.tiles.size()) - 1;
    control->switchStyle = Clamp(style_index, 0, maxStyle);
    InvalidateRect(control->window, nullptr, TRUE);
    return TRUE;
}

// Applies a clamped body style index from the loaded body atlas.
extern "C" BOOL UIToggle_SetBodyStyle(UIToggleHandle handle, int style_index)
{
    if (!IsValidHandle(handle) || !EnsureAtlasesLoaded())
    {
        return FALSE;
    }

    ToggleControl* control = handle->control;
    const int maxStyle = static_cast<int>(g_bodyAtlas.tiles.size()) - 1;
    control->bodyStyle = Clamp(style_index, 0, maxStyle);
    InvalidateRect(control->window, nullptr, TRUE);
    return TRUE;
}

extern "C" BOOL UIToggle_GetWindow(UIToggleHandle handle, HWND* out_window)
{
    if (!IsValidHandle(handle) || out_window == nullptr)
    {
        return FALSE;
    }

    *out_window = handle->control->window;
    return TRUE;
}
