# UI Toggle DLL Sample

A small C++/Win32 project that builds:

- `UIToggle.dll`: a reusable custom toggle control with a C ABI.
- `UI.exe`: a sample app that loads the DLL dynamically and demonstrates usage.

## Repository layout

- `lib/UI/include/Toggle.h` — public DLL API (opaque handle + exported functions)
- `lib/UI/src/Toggle.cpp` — internal control implementation and rendering
- `lib/UI/assets/Troggle/` — image atlases used by the toggle control
- `main.cpp` — sample Win32 host application
- `DLL_USAGE.md` — architecture and API notes

## Requirements

- CMake 3.17+
- A C++14-capable compiler
- Windows toolchain (the project targets Win32 APIs)

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

Build outputs are written to `build/bin/`:

- `UIToggle.dll`
- `UI.exe`
- `assets/`

## Run

From `build/bin`, run `UI.exe`. The sample app loads `UIToggle.dll` via `LoadLibraryW` and resolves symbols using `GetProcAddress`.

## License

This project is available under the MIT License. See [LICENSE](LICENSE).
