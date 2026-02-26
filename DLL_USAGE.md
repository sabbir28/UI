# UIToggle DLL Architecture

## Public API boundary

The DLL exposes a pure C ABI in `lib/UI/include/Toggle.h` using an opaque `UIToggleHandle`.
This keeps ABI stable across compilers and avoids STL types crossing the DLL boundary.

Key API calls:
- `UIToggle_RegisterClass`: Registers the custom control class.
- `UIToggle_Create` / `UIToggle_Destroy`: Explicit lifecycle management.
- `UIToggle_SetChecked` / `UIToggle_GetChecked`: Stable state operations.
- `UIToggle_SetSwitchStyle` / `UIToggle_SetBodyStyle`: Style selection with clamping.

## Internal structure

`lib/UI/src/Toggle.cpp` keeps all rendering and state logic private:
- `ToggleControl` owns one HWND and all mutable state.
- Atlas loading is internal and validated before control use.
- Painting and animation are managed in the control window procedure.
- Invalid handles are rejected safely by every exported API call.

## Behavior guarantees

- API functions are null-safe and return `FALSE` on invalid inputs.
- Toggle state updates are centralized in one path (`SetChecked`) to avoid drift.
- Teardown kills timers, destroys the control window, and frees owned memory.
- Style indexes are clamped to valid atlas ranges.

## Consumer example

`main.cpp` demonstrates explicit runtime loading (`LoadLibraryW` + `GetProcAddress`) and shows
state notifications via parent `WM_COMMAND` handling.
