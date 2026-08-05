# raqim

A small, keyboard-friendly calculator for Linux built with C++ and Qt 6, themed live by
[Noctalia](https://github.com/Abdogouhmad/noctalia).

## Features

- Standard 16-key layout: digits, `.`, `+/-`, `%`, `⌫`, `C`, `=`
- Full keyboard support — digits, `+ - * /`, `.`/`,`, `Enter`/`Return`/`=`, `Backspace`, `Esc`
- Operation chaining with correct precedence on repeated operations
- Division by zero shows an "Error" state instead of crashing
- **Live desktop theming**: reads Noctalia's resolved `colors.json`, applies the Material-You
  palette as a `QPalette` + stylesheet, and re-themes in real time when you switch schemes,
  wallpaper (Material You mode), or light/dark mode — no restart needed
- Animated theme transitions (320 ms ease-in-out) instead of hard color snaps

## Requirements

- CMake 3.16+
- Qt 6 (Widgets)
- A C++17 compiler

## Building

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j"$(nproc)"
```

Run it:

```sh
./build-release/raqim
```

## Theming

Noctalia writes its resolved color state to `~/.config/noctalia/colors.json` (or
`~/.config/quickshell/noctalia/colors.json`). raqim picks it up automatically and re-themes
live. If Noctalia isn't installed, raqim falls back to the default Qt palette.

## License

Distributed under the MIT license. See `LICENSE` for details.
