#pragma once

#include <windows.h>

#ifdef UI_TOGGLE_DLL_EXPORTS
#define UI_TOGGLE_API __declspec(dllexport)
#else
#define UI_TOGGLE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UIToggleHandleTag* UIToggleHandle;

typedef enum UIToggleState
{
    UI_TOGGLE_STATE_OFF = 0,
    UI_TOGGLE_STATE_ON = 1
} UIToggleState;

typedef struct UIToggleCreateParams
{
    HWND parent;
    int x;
    int y;
    int width;
    int height;
    int control_id;
    int radio_group;
} UIToggleCreateParams;

UI_TOGGLE_API BOOL UIToggle_RegisterClass(HINSTANCE instance);
UI_TOGGLE_API UIToggleHandle UIToggle_Create(const UIToggleCreateParams* params);
UI_TOGGLE_API void UIToggle_Destroy(UIToggleHandle handle);

UI_TOGGLE_API BOOL UIToggle_SetChecked(UIToggleHandle handle, BOOL checked, BOOL notify_parent);
UI_TOGGLE_API BOOL UIToggle_GetChecked(UIToggleHandle handle, BOOL* checked);
UI_TOGGLE_API BOOL UIToggle_SetSwitchStyle(UIToggleHandle handle, int style_index);
UI_TOGGLE_API BOOL UIToggle_SetBodyStyle(UIToggleHandle handle, int style_index);
UI_TOGGLE_API BOOL UIToggle_GetWindow(UIToggleHandle handle, HWND* out_window);

#ifdef __cplusplus
}
#endif
