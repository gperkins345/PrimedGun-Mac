# Primed2Gun — Metroid Prime 1 & 2 VR on macOS

This tree builds **two separate Mac apps** from one codebase, streaming to a Meta
Quest over USB via the [OXRSys](https://github.com/demonixis/OpenXR-OSX) OpenXR
runtime — no PCVR, no Windows, no SteamVR:

| Product | Game | Build flag |
|---|---|---|
| **PrimedGun-Mac** | Metroid Prime (GC, `GM8E01`) | `-DPRIMEDGUN_ENABLE_PRIME2=OFF` |
| **Primed2Gun-Mac** | Metroid Prime 2: Echoes (GC, `G2ME01`) + Prime 1 | default |

With `PRIMEDGUN_ENABLE_PRIME2=OFF`, every Prime 2 code path compiles out — the
Prime 1 product is byte-equivalent to a tree without the Prime 2 work, which keeps
it clean for upstreaming.

Prime 2 support includes: hand-aimed arm cannon (projectiles follow the hand ray),
native lock-on, HMD-relative locomotion, snap turn, gesture visor/beam switching,
the PrimedGun VR menu (left-stick press) and height recenter (right-stick press),
head-locked full-view screen effects and cutscenes, and helmet-visor rendering.

## Prerequisites

- Apple Silicon Mac, macOS 11+
- Xcode command-line tools, `cmake`, `ninja` (`brew install cmake ninja`)
- Qt 6 for macOS (point `CMAKE_PREFIX_PATH` at it, e.g. `~/Qt/6.8.3/macos`)
- Vulkan SDK / MoltenVK (`brew install molten-vk` or the LunarG SDK)
- `adb` (`brew install android-platform-tools`) with the Quest in developer mode
- The **OXRSys** runtime built for macOS and its Quest client APK installed on the
  headset (checked out as a sibling directory `../OpenXR-OSX`, or set
  `XR_RUNTIME_JSON` to your runtime manifest)

## Build

```sh
# Primed2Gun-Mac (Prime 2 + Prime 1)
cmake -B build-macos -G Ninja \
  -DPRIMEDGUN_PRODUCT_NAME=Primed2Gun-Mac \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/macos
ninja -C build-macos

# PrimedGun-Mac (Prime 1 only)
cmake -B build-macos-prime1 -G Ninja \
  -DPRIMEDGUN_PRODUCT_NAME=PrimedGun-Mac \
  -DPRIMEDGUN_ENABLE_PRIME2=OFF \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.3/macos
ninja -C build-macos-prime1
```

Binaries land in `<build-dir>/Binaries/<ProductName>`.

## Launch (VR, Quest over USB)

1. Connect the Quest over USB (accept the debugging prompt in the headset).
2. Run the launcher for the game you want:

```sh
# Metroid Prime 2: Echoes in VR
Tools/mac/run-vr.sh prime2 "/path/to/Metroid Prime 2 - Echoes (USA).rvz"

# Metroid Prime in VR
Tools/mac/run-vr.sh prime1 "/path/to/Metroid Prime (USA).rvz"
```

The script sets up the adb port reverses, (re)starts the OXRSys client on the
headset, and boots the game in OpenXR stereo. Put the headset on; press Ctrl-C in
the terminal to stop. Each product keeps its own user directory
(`vr-user-mac-prime1` / `vr-user-mac-prime2`), so their settings never collide.

To launch flat (no VR) for testing, run the binary directly:

```sh
build-macos/Binaries/Primed2Gun-Mac -e "/path/to/game.rvz" -u /tmp/p2-user \
  -C GFX.VR.EnableOpenXR=False
```

## In-headset controls (Prime 2)

- **Right controller** aims the arm cannon; trigger fires
- **Left stick** moves relative to where you look/point; **right stick** snap-turns
- **Off-hand near head + stick flick** switches visors; **cannon hand near head +
  stick flick** switches beams
- **Left stick click** opens the PrimedGun VR menu (calibration, layout, movement)
- **Right stick click** recenters your height
