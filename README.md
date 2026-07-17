# PrimedGun-Mac — Metroid Prime VR on macOS

A macOS build of **[PrimedGun](https://github.com/Nobbie248/PrimedGun)** (a Dolphin
fork that adds motion-controlled VR to Metroid Prime), streaming to a **Meta Quest
over USB** via the [OXRSys](https://github.com/demonixis/OpenXR-OSX) OpenXR runtime —
**no PCVR, no Windows, no SteamVR.**

This fork is **Metroid Prime 1 only** (GameCube, `GM8E01`). It carries the macOS/
MoltenVK-specific work needed to run PrimedGun natively on Apple Silicon (multiview
stereo, the menu-map depth fix, 3D cutscene rendering, color pipeline). For Metroid
Prime 2: Echoes support, see the separate
[Primed2Gun](https://github.com/gperkins345/Primed2Gun) project.

## What you need

- **Apple Silicon Mac**, macOS 11 or newer
- **Xcode command-line tools** (`xcode-select --install`)
- **cmake** and **ninja** — `brew install cmake ninja`
- **Qt 6 for macOS** — install from the Qt online installer (e.g. `~/Qt/6.8.3/macos`)
- **MoltenVK / Vulkan SDK** — `brew install molten-vk`, or the LunarG Vulkan SDK
- **adb** — `brew install android-platform-tools`, with the Quest in developer mode
- The **OXRSys runtime** built for macOS, and its **Quest client app** installed on
  the headset. Follow [OpenXR-OSX](https://github.com/demonixis/OpenXR-OSX); note the
  path to its `oxrsys-runtime.json`.
- A **Metroid Prime (USA) GameCube disc image** you dumped yourself (`.rvz`/`.iso`),
  game ID `GM8E01`.

## Build

```sh
git clone https://github.com/gperkins345/PrimedGun-Mac.git
cd PrimedGun-Mac
git submodule update --init --recursive

cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/macos
ninja -C build
```

The app binary is `build/Binaries/PrimedGun`.

## Run (VR, streaming to the Quest over USB)

1. Connect the Quest by USB and accept the on-headset debugging prompt.
2. Point the OpenXR loader at your OXRSys runtime and start the Quest client:

```sh
export XR_RUNTIME_JSON=/path/to/OpenXR-OSX/build/macos-arm64/runtime/oxrsys-runtime.json

# forward the streaming ports and (re)launch the headset client
for p in 9944 9945 9946; do adb reverse tcp:$p tcp:$p; done
adb shell am force-stop net.demonixis.oxrsys.android
adb shell am start -n net.demonixis.oxrsys.android/com.oculus.NativeActivity
```

3. Launch the game in VR:

```sh
build/Binaries/PrimedGun -b -v Vulkan \
  -e "/path/to/Metroid Prime (USA).rvz" \
  -u ~/Library/Application\ Support/PrimedGun-Mac \
  -C GFX.Stereoscopy.StereoMode=6 \
  -C GFX.VR.EnableOpenXR=True
```

Put the headset on. Press `Ctrl-C` in the terminal to stop.

There's also a convenience launcher that does the adb setup for you:

```sh
XR_RUNTIME_JSON=/path/to/oxrsys-runtime.json \
  Tools/mac/run-vr.sh "/path/to/Metroid Prime (USA).rvz"
```

To run flat (no headset, for testing) drop the VR flags:

```sh
build/Binaries/PrimedGun -e "/path/to/Metroid Prime (USA).rvz" \
  -C GFX.VR.EnableOpenXR=False
```

## In-headset controls

PrimedGun's motion controls apply: aim the arm cannon with your controller and pull
the trigger to fire, motion-based visor/beam handling, one-click height calibration
(right-stick press), and the in-headset settings menu (left-stick press). See the
[upstream documentation](README-upstream.md) for the full control scheme, calibration
options, save transfer, and multiworld support.

## Credits

- **macOS port** by **Ceiling**.
- Metroid Prime VR by **[PrimedGun](https://github.com/Nobbie248/PrimedGun)** (Nobbie),
  built on the **Dolphin** emulator (Dolphin ReduX by iChris4).
- See the [upstream documentation](README-upstream.md) for the full PrimedGun credits.

Not affiliated with Nintendo or Retro Studios; bring your own legally-dumped disc image.
