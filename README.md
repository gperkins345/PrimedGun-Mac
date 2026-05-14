# PrimedGun

PrimedGun is a Dolphin OpenXR-side VR enhancement app for Metroid Prime running in Dolphin. It reads OpenXR tracking through a Dolphin hook, writes the arm cannon and input state into game memory, and applies the small set of game patches PrimedGun needs while the game is loaded.

## Requirements

- Windows 10/11
- CMake 3.16+
- A C++ compiler toolchain
- OpenXR SDK headers at `C:\OpenXR-SDK`
- Dolphin running Metroid Prime GCN NTSC Rev 0 (`GM8E01`)
- Built and tested with Dolphin-OpenXR 2512-421 -dirty

## Build

```bat
git clone --recurse-submodules https://github.com/Nobbie248/PrimedGun.git
cd PrimedGun
mkdir build
cd build
cmake .. -G Ninja
cmake --build . --config Release
```

## Runtime Files

For distribution/share, the useful files are:

- `PrimedGun.exe`
- `PrimedGun_DolphinHook.dll`
- `assets\gecko\GM8E01_PrimedGun.ini`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`
- `primedgun_settings.ini`

## Features

- Full directional movement.
- Offhand controller yaw controls directional strafing.
- Visor hand gesture input: the app reads the control stick as visor D-pad input when the controller is near your head.
- Improved gun-based lock/scan targeting.
- VR arm cannon tracking through Dolphin-side OpenXR.
- In-headset settings menu anchored to the left controller.
- Dolphin-side hook bridge for app-owned game patches, OpenXR tracking, overlays, and diagnostics.
- Automatic Dolphin XR setup for the GM8E01 VR settings PrimedGun expects.
- Pre-configured shader profile.

## Dolphin Setup

PrimedGun configures the Dolphin pieces it needs at startup where possible:

- Sets the GM8E01 Dolphin XR camera forward offset to `0`.
- Sets the GM8E01 Dolphin XR world scale / units-per-meter value used by PrimedGun.
- Can map Dolphin's Port 1 controls and reset-view hotkey to OpenXR when the setting is enabled.
- Disables unmanaged Dolphin Action Replay and Gecko enabled-code lists so old user codes do not fight PrimedGun.

## App-Owned Patches

The hook loads PrimedGun patches from:

```text
assets\gecko\GM8E01_PrimedGun.ini
```

These patches are owned by PrimedGun and are applied through the hook, not through Dolphin's normal user-managed AR/Gecko enabled-code flow. Current patch areas include culling, VR rotation restore, lock/scan targeting, visor input support, and arm cannon idle fidget disable.

## Usage

1. Launch `PrimedGun.exe`.
2. Load Metroid Prime GCN NTSC Rev 0 (`GM8E01`) in Dolphin.
3. PrimedGun detects the game when GM8E01 is loaded into memory.
4. Click the right stick to set height.

The alignment prompt can show again after the game is unloaded from memory and loaded again. Dolphin itself does not need to be closed.

## VR Settings Menu

- Click the left thumbstick to open or close the in-headset settings menu.
- The menu is attached to the left controller.
- Aim with the right-controller laser.
- Use right trigger or A to change the pointed-at setting.
- Use `Save Settings` to write `primedgun_settings.ini`.
- Use `Reset All Settings` to restore PrimedGun defaults.
- Each settings tab also has its own reset row.

## Notes

- Settings are saved to `primedgun_settings.ini`.
- PrimedGun currently expects Dolphin's user config at `C:\Users\<user>\Documents\Dolphin Emulator\...` for temporary controller/hotkey setup.
- The app reads controller tracking from Dolphin-side OpenXR and writes the cannon transform into Dolphin memory.
- PrimedGun should be running before Metroid Prime is loaded so the Dolphin-side hook is ready as soon as GM8E01 memory appears.
- For the best experience, try not to turn your body around. You can move in the game like this, but functionality is not ideal.

For further enhancements to your VR experience, join the Dolphin VR Discord.

- By Nobbie

Thank you to the Metroid Prime modding community for the resources and research that helped make this possible.
