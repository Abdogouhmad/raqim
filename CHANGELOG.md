# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.1.0] - 2026-08-06

### Added

- Live expression display: the full equation stays visible as you type
  (`10+10`, `1000+10%`) instead of only the running result.
- `%` as an inline postfix operator with classic calculator behaviour:
  `1000+10%` evaluates to `1100` (10% of the preceding value), `200−50%` to
  `100`, and a leading `50%` to `0.5`. Inside `*` and `/` it means `/100`
  (`1000*10%` → `100`).
- A rewritten expression parser with proper operator precedence (`*` and `/`
  bind tighter than `+` and `-`) and unary minus support.
- Keyboard support for `%` (`Shift+5` / keypad).

### Changed

- Window now uses a fixed 400×600 size.
- Window title no longer embeds the version string.

### Fixed

- Pressing `%` corrupted the inline math display; the symbol now renders
  inline and computes correctly.
- Typing a digit right after a `%`-marked number starts a fresh number
  instead of producing a malformed expression.

## [1.0.0] - 2026-08-05

### Added

- Standard 16-key calculator UI (0-9, `.`, `+/-`, `%`, `⌫`, `C`, `=`) with a
  read-only display.
- Full keyboard support: digits, `+ - * /`, `.`/`,`, `Enter`/`Return`/`=`,
  `Backspace`, and `Esc` to clear.
- Operation chaining with proper operator precedence on repeated operations.
- Error handling for division by zero (displays "Error").
- Live desktop theming via [Noctalia](https://github.com/Abdogouhmad/noctalia):
  reads `colors.json`, applies the resolved Material-You palette as a
  `QPalette` + stylesheet, and re-themes in real time on scheme or wallpaper
  changes.
- Animated theme transitions (320ms ease-in-out) instead of hard color snaps.

### Fixed

- Duplicate `ThemeManager` definition that broke the build (class was
  accidentally embedded in `calculatorwindow.h`).
- Missing `CalculatorWindow` class declaration caused by the header mix-up.
- Dangling layout/connection code that sat outside `buildUi()`.
