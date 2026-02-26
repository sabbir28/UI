// Toggle.cpp
#include "../include/Toggle.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#include <windows.h>
#include <wingdi.h>
#include <string>
#include <stdexcept>
#include <algorithm>

#pragma comment(lib, "msimg32.lib")

namespace UI
{
    static constexpr wchar_t TOGGLE_CLASS_NAME[] = L"UI_TOGGLE";

    namespace
    {
        ImageAtlas g_bodyAtlas;   // 10 styles (5x2)
        ImageAtlas g_switchAtlas; // 6 styles (3x2)

        // ==========================
        // C++14 SAFE CLAMP
        // ==========================
        template<typename T>
        T Clamp(T value, T min, T max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }

        RECT GetVisibleBounds(const ImageAtlas& atlas, int index)
        {
            RECT result{ 0,0,0,0 };

            const auto& t = atlas.tiles[index];

            int minX = t.w;
            int minY = t.h;
            int maxX = 0;
            int maxY = 0;

            bool found = false;

            for (int y = 0; y < t.h; ++y)
            {
                for (int x = 0; x < t.w; ++x)
                {
                    int pxIndex =
                            ((t.y + y) * atlas.width + (t.x + x)) * 4;

                    unsigned char alpha = atlas.pixels[pxIndex + 3];

                    if (alpha > 0)
                    {
                        found = true;
                        minX = std::min(minX, x);
                        minY = std::min(minY, y);
                        maxX = std::max(maxX, x);
                        maxY = std::max(maxY, y);
                    }
                }
            }

            if (!found)
            {
                RECT r;
                r.left   = t.x;
                r.top    = t.y;
                r.right  = t.x + t.w;
                r.bottom = t.y + t.h;
                return r;
            }

            result.left   = t.x + minX;
            result.top    = t.y + minY;
            result.right  = t.x + maxX + 1;
            result.bottom = t.y + maxY + 1;

            return result;
        }

        std::wstring exe_dir()
        {
            wchar_t buf[MAX_PATH]{};
            GetModuleFileNameW(nullptr, buf, MAX_PATH);
            std::wstring p(buf);
            return p.substr(0, p.find_last_of(L"\\/"));
        }

        std::wstring assets_dir()
        {
            auto p = exe_dir();
            p = p.substr(0, p.find_last_of(L"\\/"));
            return p + L"\\data\\assets\\Troggle";
        }

        std::string w2u8(const std::wstring& w)
        {
            if (w.empty()) return {};

            int size = WideCharToMultiByte(
                    CP_UTF8, 0,
                    w.c_str(), (int)w.size(),
                    nullptr, 0, nullptr, nullptr
            );

            std::string s(size, '\0');

            WideCharToMultiByte(
                    CP_UTF8, 0,
                    w.c_str(), (int)w.size(),
                    &s[0], size, nullptr, nullptr
            );

            return s;
        }

        ImageAtlas LoadAtlas(const std::wstring& path, int cols, int rows)
        {
            ImageAtlas a;
            std::string p = w2u8(path);

            int ch = 0;
            unsigned char* data = stbi_load(p.c_str(), &a.width, &a.height, &ch, 4);

            if (!data)
                throw std::runtime_error("Atlas load failed");

            a.pixels.assign(data, data + a.width * a.height * 4);
            stbi_image_free(data);

            // Premultiply alpha (REQUIRED for AlphaBlend)
            for (int i = 0; i < a.width * a.height; ++i)
            {
                unsigned char* px = &a.pixels[i * 4];

                float alpha = px[3] / 255.0f;

                px[0] = (unsigned char)(px[0] * alpha); // R
                px[1] = (unsigned char)(px[1] * alpha); // G
                px[2] = (unsigned char)(px[2] * alpha); // B
            }

            const int tileW = a.width  / cols;
            const int tileH = a.height / rows;

            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                    a.tiles.push_back({ c * tileW, r * tileH, tileW, tileH });

            return a;
        }

        void EnsureAtlases()
        {
            if (!g_bodyAtlas.pixels.empty())
                return;

            const auto base = assets_dir();

            g_bodyAtlas   = LoadAtlas(base + L"\\switch-body.png", 5, 2);
            g_switchAtlas = LoadAtlas(base + L"\\Switch.png", 3, 2);
        }

        void DrawTile(HDC hdc, const RECT& dst,const ImageAtlas& atlas, int index)
        {
            if (index < 0 || index >= (int)atlas.tiles.size())
                return;

            RECT crop = GetVisibleBounds(atlas, index);

            int srcX = crop.left;
            int srcY = crop.top;
            int srcW = crop.right - crop.left;
            int srcH = crop.bottom - crop.top;

            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = atlas.width;
            bmi.bmiHeader.biHeight = -atlas.height; // top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* bits = nullptr;
            HBITMAP bmp = CreateDIBSection(
                    hdc,
                    &bmi,
                    DIB_RGB_COLORS,
                    &bits,
                    nullptr,
                    0
            );

            if (!bmp)
                return;

            memcpy(bits, atlas.pixels.data(),
                   atlas.width * atlas.height * 4);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(memDC, bmp);

            BLENDFUNCTION blend{};
            blend.BlendOp = AC_SRC_OVER;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;

            AlphaBlend(
                    hdc,
                    dst.left,
                    dst.top,
                    dst.right - dst.left,   // FULL user width
                    dst.bottom - dst.top,   // FULL user height
                    memDC,
                    srcX,
                    srcY,
                    srcW,
                    srcH,
                    blend
            );

            SelectObject(memDC, old);
            DeleteObject(bmp);
            DeleteDC(memDC);
        }
    }

    static constexpr UINT TIMER_ANIM = 1;

    void Toggle::SetSwitchStyle(int index)
    {
        EnsureAtlases();

        m_switchStyle = Clamp(
                index,
                0,
                (int)g_switchAtlas.tiles.size() - 1
        );

        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void Toggle::SetBodyStyle(int index)
    {
        EnsureAtlases();

        m_bodyStyle = Clamp(
                index,
                0,
                (int)g_bodyAtlas.tiles.size() - 1
        );

        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void Toggle::Register(HINSTANCE hInst)
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc   = Toggle::WindowProc;
        wc.hInstance     = hInst;
        wc.lpszClassName = TOGGLE_CLASS_NAME;
        wc.hCursor       = LoadCursor(nullptr, IDC_HAND);
        RegisterClassW(&wc);
    }

    HWND Toggle::Create(HWND parent,
                        int x, int y, int w, int h,
                        int id, int radioGroup)
    {
        EnsureAtlases();

        m_radioGroup = radioGroup;

        m_hwnd = CreateWindowExW(
                0,
                TOGGLE_CLASS_NAME,
                L"",
                WS_CHILD | WS_VISIBLE,
                x, y, w, h,
                parent,
                (HMENU)(intptr_t)id,
                GetModuleHandleW(nullptr),
                this
        );

        return m_hwnd;
    }

    bool Toggle::IsChecked() const
    {
        return m_state == ToggleState::On;
    }

    void Toggle::SetChecked(bool v, bool notify)
    {
        m_state = v ? ToggleState::On : ToggleState::Off;

        const int travel = g_switchAtlas.tiles[0].w;
        m_targetOffset = v ? travel : 0;

        SetTimer(m_hwnd, TIMER_ANIM, 10, nullptr);

        if (notify)
        {
            SendMessageW(
                    GetParent(m_hwnd),
                    WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(m_hwnd), BN_CLICKED),
                    (LPARAM)m_hwnd
            );
        }
    }

    void Toggle::OnClick()
    {
        SetChecked(m_state == ToggleState::Off);
    }

    void Toggle::AnimateStep()
    {
        constexpr int speed = 4;

        if (m_knobOffset < m_targetOffset)
            m_knobOffset = std::min(m_knobOffset + speed, m_targetOffset);
        else if (m_knobOffset > m_targetOffset)
            m_knobOffset = std::max(m_knobOffset - speed, m_targetOffset);
        else
            KillTimer(m_hwnd, TIMER_ANIM);

        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void Toggle::OnPaint()
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rc{};
        GetClientRect(m_hwnd, &rc);

        DrawTile(hdc, rc, g_bodyAtlas, m_bodyStyle);

        RECT knob = rc;
        knob.left  += m_knobOffset;
        knob.right += m_knobOffset;

        DrawTile(hdc, knob, g_switchAtlas, m_switchStyle);

        EndPaint(m_hwnd, &ps);
    }

    LRESULT CALLBACK Toggle::WindowProc(
            HWND hwnd, UINT msg,
            WPARAM wParam, LPARAM lParam)
    {
        Toggle* self = nullptr;

        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<Toggle*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)self);
            self->m_hwnd = hwnd;
        }

        self = reinterpret_cast<Toggle*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!self)
            return DefWindowProcW(hwnd, msg, wParam, lParam);

        switch (msg)
        {
            case WM_LBUTTONDOWN: self->OnClick(); return 0;
            case WM_TIMER:       self->AnimateStep(); return 0;
            case WM_PAINT:       self->OnPaint(); return 0;
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}