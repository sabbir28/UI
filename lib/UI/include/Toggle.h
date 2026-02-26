#pragma once
#include <windows.h>
#include <vector>


namespace UI
{
    struct TileRect
    {
        int x, y, w, h;
    };

    struct ImageAtlas
    {
        int width  = 0;
        int height = 0;

        std::vector<unsigned char> pixels;
        std::vector<TileRect> tiles;
    };

    enum class ToggleState
    {
        Off = 0,
        On  = 1
    };

    class Toggle
    {
    public:
        static void Register(HINSTANCE);

        HWND Create(
                HWND parent,
                int x, int y, int w, int h,
                int id,
                int radioGroup = -1
        );

        // Logic
        bool IsChecked() const;
        void SetChecked(bool value, bool notify = true);

        // 🔥 STYLE CONTROL (NEW)
        void SetSwitchStyle(int index);  // 0–5
        void SetBodyStyle(int index);    // 0–9

    private:
        static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

        void OnPaint();
        void OnClick();
        void AnimateStep();

        HWND m_hwnd = nullptr;

        ToggleState m_state = ToggleState::Off;

        int m_knobOffset   = 0;
        int m_targetOffset = 0;
        int m_radioGroup   = -1;

        // 🔥 STYLE MEMBERS (NEW)
        int m_switchStyle = 0; // 6 switch tiles
        int m_bodyStyle   = 0; // 10 background tiles
    };
}