# CLAUDE.md

ZMK firmware config for a **Corne Choc Pro BT** (Keebart, nRF52840 M.2 Module) with a custom French AZERTY layout.

## Build

GitHub Actions builds on every push/PR (`.github/workflows/build.yml` → ZMK reusable workflow, matrix in `build.yaml`). Firmware is downloaded from the Actions run artifacts.

## Where things live

- `config/corne_choc_pro.keymap` — layers, combos, conditional layers, behaviors, macros
- `config/keys_fr.h` — `FR_*` AZERTY keycodes; always use these instead of raw ZMK keycodes for characters
- `boards/keebart/corne_choc_pro/` — custom board definition (Zephyr HWMv2, build targets `corne_choc_pro_{left,right}//zmk`)
- `src/behaviors/` + `dts/bindings/behaviors/` — custom `bat_rgb` / `mode_rgb` behaviors, wired in via `CMakeLists.txt`, `Kconfig`, `zephyr/module.yml`

## Gotchas

- ZMK is pinned to a `main` commit SHA (Zephyr 4.1); `config/west.yml` and `.github/workflows/build.yml` must point at the same SHA. Bump both together. There is no ZMK release past v0.3 that supports Zephyr 4.1 yet.
- Local build (podman/docker, `zmkfirmware/zmk-build-arm:stable`), output checks and Renode emulation: see `docs/building-locally.md`. Keep it in sync with `build.yaml` when targets change.
- No displays: this variant has 4 extra keys in place of the nice!view screens. Key positions are 0–45 in rows of 14 / 14 / 12 / 6.
- Every combo has an explicit `layers` list, and some layers (6, 7, 9, 10) are activated via `conditional_layers`. When adding or renumbering layers, update all combo `layers` lists and the `conditional_layers` block. Inline comments about layer ranges in the combos block may be stale — trust the `layers` property.
- Upstream (Keebart template, remote `upstream`) is still on ZMK v0.3 with the old HWMv1 `boards/arm/` layout, so don't merge it. Cherry-pick board-level fixes by hand into `boards/keebart/`. Its other boards (Piantor, Sofle, 5col) and display shields don't apply here.
- `my_keymap.svg` is generated with https://keymap-drawer.streamlit.app/ from the keymap + `config/corne_choc_pro.json`; regenerate after layout changes.
