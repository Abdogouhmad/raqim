# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
