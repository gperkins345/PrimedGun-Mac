# PrimedGun

![PrimedGun gameplay](assets/readme/primedgun-hero.jfif)

PrimedGun is a Dolphin ReduX-based build focused on improving Metroid Prime's VR experience.

## Build - Windows

Use a Visual Studio x64 developer environment, then build the PrimedGun executable. Git, the latest Windows SDK, CMake, and Ninja should be installed.

```bat
git clone --recurse-submodules https://github.com/Nobbie248/PrimedGun.git
cd PrimedGun
git submodule update --init --recursive
cmake -S . -B build\Release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build\Release --target dolphin-emu
```

The built app is written to `Binary\x64\PrimedGun.exe`.

## Build - Native Linux

```bash
git clone --recurse-submodules https://github.com/Nobbie248/PrimedGun.git
cd PrimedGun
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_VR=ON -DENABLE_VULKAN=ON
cmake --build build --target dolphin-emu
cmake --install build --prefix /usr/local
```

## Build - Linux Flatpak

The Flatpak package is built from `Flatpak/org.PrimedGun.PrimedGun.yml`. The
manifest builds PrimedGun, installs the PrimedGun launcher wrapper, packages the
desktop UI images into `/app/bin/assets`, and packages the runtime files needed
by the application.

Install Flatpak Builder and the KDE runtime dependencies, then build from the
repository root:

```bash
flatpak install flathub org.kde.Platform//6.10 org.kde.Sdk//6.10
flatpak-builder --user --force-clean --repo=flatpak-repo flatpak-build Flatpak/org.PrimedGun.PrimedGun.yml
flatpak build-bundle flatpak-repo PrimedGun.flatpak org.PrimedGun.PrimedGun
```

Install or replace the local Flatpak bundle with:

```bash
flatpak install --user --reinstall PrimedGun.flatpak
flatpak run org.PrimedGun.PrimedGun
```

Flatpak user files are not written beside the executable. PrimedGun's writable
Flatpak folders are:

- Config and INI files: `~/.var/app/org.PrimedGun.PrimedGun/config/PrimedGun/`
- User data, game settings, textures, resource packs, and memory cards:
  `~/.var/app/org.PrimedGun.PrimedGun/data/PrimedGun/`

PrimedGun's default Dolphin, VR, and Metroid Prime settings are built into the
application. On a clean first launch, those defaults are used automatically. If
user INI files already exist, the user's saved settings are respected instead of
being overwritten.

If an older broken Flatpak already created bad default settings, reinstalling the
bundle may not be enough. To force a clean Flatpak sandbox, run:

```bash
flatpak uninstall --delete-data org.PrimedGun.PrimedGun
flatpak install --user PrimedGun.flatpak
```

This deletes the Flatpak sandbox data for PrimedGun, including saves and local
settings, so back up memory cards first if needed.

## Runtime Files

For distribution, use the contents of `Binary\x64`. The important runtime pieces are:

- `PrimedGun.exe`
- `assets/`
- `Licenses/`
- `Sys/`
- `User/` for runtime data and packaged PrimedGun texture assets
- `QtPlugins/`
- `COPYING`
- `qt.conf`
- `Qt6Core.dll`
- `Qt6Gui.dll`
- `Qt6Svg.dll`
- `Qt6Widgets.dll`

## Features

- Full directional movement.
- Modern VR control scheme.
- Visor hand gesture input.
- Improved gun-based lock/scan targeting.
- VR arm cannon tracking.
- One-click height calibration.
- Cannon position, rotation calibration.
- Easy cannon texture swapping tool.
- In-headset settings menu.

## Credits

- Created by Nobbie.
- Thank you to the Metroid Prime modding community for the resources and research that helped make this possible.
- Huge thank you to iChris4 for Dolphin ReduX development.
- Thank you to the early testers: GeekyGami, Lucaspec72, TorchRing, detective_yoshi, PHA3ESH1FTGAMES, retrovideogamer, Samevi, Mochu, VideoGameEsoterica and VRified Games.
- For further enhancements to your VR experience, join the Dolphin VR Discord: https://discord.gg/GdmffzCTrh
