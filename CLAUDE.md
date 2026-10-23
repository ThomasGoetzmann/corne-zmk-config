# CLAUDE.md

ZMK firmware config for a **Corne Choc Pro BT** (Keebart, nRF52840 M.2 Module) with a custom French AZERTY layout.

## Build

No local build. GitHub Actions builds on every push/PR (`.github/workflows/build.yml` → ZMK v0.3 reusable workflow, matrix in `build.yaml`). Firmware is downloaded from the Actions run artifacts.

## Where things live

- `config/corne_choc_pro.keymap` — layers, combos, conditional layers, behaviors, macros
- `config/keys_fr.h` — `FR_*` AZERTY keycodes; always use these instead of raw ZMK keycodes for characters
- `boards/arm/corne_choc_pro/` — custom board definition
- `src/behaviors/` + `dts/bindings/behaviors/` — custom `bat_rgb` / `mode_rgb` behaviors, wired in via `CMakeLists.txt`, `Kconfig`, `zephyr/module.yml`

## Gotchas

- No displays: this variant has 4 extra keys in place of the nice!view screens. Key positions are 0–45 in rows of 14 / 14 / 12 / 6.
- Every combo has an explicit `layers` list, and some layers (6, 7, 9, 10) are activated via `conditional_layers`. When adding or renumbering layers, update all combo `layers` lists and the `conditional_layers` block. Inline comments about layer ranges in the combos block may be stale — trust the `layers` property.
- Upstream (Keebart template) merges may reintroduce files for other boards (Piantor, Sofle) or display shields — they don't apply here.
- `my_keymap.svg` is generated with https://keymap-drawer.streamlit.app/ from the keymap + `config/corne_choc_pro.json`; regenerate after layout changes.
