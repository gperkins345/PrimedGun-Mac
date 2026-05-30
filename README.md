# PrimedGun

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

## Build - Linux

PrimedGun is based on Dolphin, so Linux builds follow Dolphin's CMake flow. Install a recent GCC or Clang with C++23 support, plus CMake, Ninja, Qt 6, SDL2, libevdev, libudev, Vulkan development files, and OpenXR development files. CMake will report any missing packages, and some libraries can be built from the bundled externals.

On Ubuntu/Debian, the dependency list is roughly:

```bash
sudo apt update
sudo apt install git cmake ninja-build build-essential pkg-config \
  qt6-base-dev qt6-base-dev-tools libqt6svg6-dev \
  libsdl2-dev libevdev-dev libudev-dev libvulkan-dev glslang-tools \
  libxrandr-dev libxi-dev libx11-dev libxext-dev libbluetooth-dev
```

Desktop Linux VR requires Vulkan and a working OpenXR runtime such as Monado or SteamVR.

Start by cloning and pulling submodules:

```bash
git clone --recurse-submodules https://github.com/Nobbie248/PrimedGun.git
cd PrimedGun
git submodule update --init --recursive
```

For a normal system install:

```bash
mkdir build
cd build
cmake .. -DENABLE_VR=ON -DENABLE_VULKAN=ON
make -j "$(nproc)"
sudo make install
```

For a local development build that does not require root:

```bash
mkdir Build
cd Build
cmake .. -DLINUX_LOCAL_DEV=true -DENABLE_VR=ON -DENABLE_VULKAN=ON
make -j "$(nproc)"
ln -s ../../Data/Sys Binaries/
```

The local build places the app under the build directory's `Binaries` folder.

## Build - Android APK

The Android project lives in `Source/Android`. Android apps are built with Gradle, and the native Dolphin/PrimedGun component is built by CMake during the Gradle build.

Install Android Studio, JDK 17, CMake, Ninja, Android SDK 36, and Android NDK `29.0.14206865`. Android Studio can install most SDK components automatically when the project is opened.

For Quest APK notes, caveats, and the exact debug APK command, see `docs/Building-Quest-APK.md`.

Make sure submodules and Android externals are available before building:

```bash
git submodule update --init --recursive
```

To build from Android Studio:

1. Open `Source/Android`.
2. Let Gradle sync and download the requested SDK/NDK/CMake components.
3. Use **Build > Generate App Bundles or APKs > Generate APKs** to produce an APK.

To build a standard debug APK from the command line:

```bat
cd Source\Android
gradlew.bat :app:assembleDebug
```

For a standard signed release APK, provide Gradle signing properties and run:

```powershell
cd Source\Android
.\gradlew.bat app:assembleRelease -Pkeystore=C:\path\release.jks -Pstorepass=<password> -Pkeyalias=<alias> -Pkeypass=<password>
```

APK outputs are written under `Source\Android\app\build\outputs\apk`. Debug APKs are signed with Android's debug key and can be sideloaded for testing.

## Runtime Files

For distribution, use the contents of `Binary\x64`. The important runtime pieces are:

- `PrimedGun.exe`
- `assets/`
- `Licenses/`
- `Sys/`
- `User/`
- `QtPlugins/`
- `COPYING`
- `README.md`
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
- In-headset settings menu.

## Credits

- Created by Nobbie.
- Thank you to the Metroid Prime modding community for the resources and research that helped make this possible.
- Huge thank you to iChris4 for Dolphin ReduX development.
- Thank you to the early testers: GeekyGami, Lucaspec72, TorchRing, detective_yoshi, PHA3ESH1FTGAMES, retrovideogamer, Samevi, Mochu, VideoGameEsoterica and VRified Games.
- For further enhancements to your VR experience, join the Dolphin VR Discord: https://discord.gg/GdmffzCTrh
