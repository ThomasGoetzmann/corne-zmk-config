# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is a ZMK firmware configuration for a **Corne Choc Pro BT** (42-key split keyboard from Keebart) with a custom French (AZERTY) layout. The keyboard uses an nRF52840 M.2 Module and connects wirelessly via Bluetooth.

## Build System

Builds are handled entirely by **GitHub Actions** — there is no local build setup. Push to the `main` branch to trigger a firmware build.

- Workflow: `.github/workflows/build.yml` — delegates to ZMK's reusable workflow at `zmkfirmware/zmk` v0.3
- Build matrix: `build.yaml` — defines left/right halves + settings_reset variants, all with `studio-rpc-usb-uart` snippet and `nice_view rgbled_adapter` shields

To get firmware: push changes → GitHub Actions → download artifact from the Actions run.

## Key Files

| File | Purpose |
|------|---------|
| `config/corne_choc_pro.keymap` | Main keymap — all layers, combos, behaviors, macros |
| `config/keys_fr.h` | French AZERTY key definitions (FR_* macros) |
| `config/west.yml` | ZMK dependency pinning (ZMK v0.3 + zmk-rgbled-widget) |
| `build.yaml` | GitHub Actions build matrix |
| `boards/arm/corne_choc_pro/` | Custom board definition (DTS, Kconfig, etc.) |
| `boards/shields/nice_view_disp/` | Custom nice!view display widget with status/art/battery |

## Keymap Architecture

The keymap (`config/corne_choc_pro.keymap`) has 5 layers:

- **Layer 0** (`default`): AZERTY layout with homerow mods on QSDF (left) and ,;:! (right). Uses `mt` (mod-tap) with `tap-preferred` flavor and 130ms tapping term.
- **Layer 1** (`numbers`): F-keys on left, numpad-style numbers on right
- **Layer 2** (`symbols`): Symbols optimized for programming
- **Layer 3** (`arrows`): Navigation (arrows, page up/down, home/end) + F-keys
- **Layer 4** (`system`): Bluetooth selection/clear, RGB controls, media keys

Layer access: `mo 1` / `mo 2` / `mo 3` via thumb keys; `mo 4` via top-left key.

**Homerow mods** (left hand): LALT on Q, LSHIFT on S, LGUI on D, LCTRL on F
**Homerow mods** (right hand): LCTRL on comma, LGUI on semicolon, LSHIFT on colon, LALT on exclamation

**Combos** (defined with `require-prior-idle-ms = <150>` to avoid false triggers with homerow mods):
- French accented characters: various key pairs produce è, é, ê, à, â, ç, ù
- Brackets/braces/parentheses: adjacent key pairs on top/bottom rows
- ESC: positions 19+22 (G+H area)

## French Key Constants

All French keycodes use the `FR_` prefix defined in `config/keys_fr.h`. This file maps AZERTY positions to ZMK HID usages. Common short aliases exist (e.g., `FR_LPAR` = `FR_LEFT_PARENTHESIS`, `FR_LBKT` = `FR_LEFT_BRACKET`).

## Keymap Layout Visualization

The SVG `my_keymap.svg` shows the current layout. To regenerate it, use https://keymap-drawer.streamlit.app/ with `config/corne_choc_pro.json` for the physical layout and the keymap file.

Online keymap editor: https://nickcoutsos.github.io/keymap-editor/
