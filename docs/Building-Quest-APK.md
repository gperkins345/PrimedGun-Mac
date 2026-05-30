# Building a Quest APK

This builds the Android APK from the Dolphin/PrimedGun Android project in `Source/Android`.

The APK is mainly useful for Quest 2/Quest 3 sideload testing. The normal debug build is signed with Android's debug key, so it can be installed with SideQuest or `adb install` without a release keystore.

## Requirements

- Android Studio or Android command-line tools
- JDK 17
- Android SDK platform 36
- Android SDK Build Tools 35
- Android NDK `29.0.14206865`
- CMake and Ninja
- Git submodules/external dependencies present

If Gradle reports missing SDK, build tools, or NDK packages, let Android Studio install them or run the build once with accepted Android SDK licenses. The Android Gradle plugin can install the requested NDK automatically when the SDK is configured correctly.

## Build

From the repository root:

```powershell
cd Source\Android
.\gradlew.bat :app:assembleDebug --no-daemon
```

The APK is written to:

```text
Source\Android\app\build\outputs\apk\debug\app-debug.apk
```

For a tester-friendly copy:

```powershell
Copy-Item Source\Android\app\build\outputs\apk\debug\app-debug.apk `
  Binary\PrimedGun-Quest-debug.apk -Force
```

## Verified Test Build

The first local Quest test APK was built with:

```powershell
$env:JAVA_HOME = "C:\PrimedGun\build\jdk17\jdk-17.0.19+10"
$env:Path = "$env:JAVA_HOME\bin;$env:Path"
cd C:\PrimedGun\Source\Android
.\gradlew.bat :app:assembleDebug --no-daemon
```

The copied test artifact was:

```text
C:\PrimedGun\Binary\PrimedGun Quest 2 debug.apk
```

Signature verification passed with Android's debug signing key:

```powershell
apksigner verify --verbose "C:\PrimedGun\Binary\PrimedGun Quest 2 debug.apk"
```

## Quest/Vulkan Caveats

- The debug APK package name is `org.dolphinemu.dolphinemu.debug`, so it installs separately from a release package.
- The app label may still show as `Dolphin Debug` unless the Android app resources are renamed.
- The Android Quest path is Vulkan/OpenXR-focused. The Quest settings code defaults the backend to Vulkan and enables OpenXR launch behavior.
- Vulkan overlay rendering should use the same PrimeGun overlay path as the recent Vulkan upload fix, but it still needs real Quest hardware testing.
- The APK includes `arm64-v8a` and `x86_64`; Quest uses `arm64-v8a`.
- Android RetroAchievements support may be disabled in this configuration. JNI calls are guarded so the build can compile without the RetroAchievements feature.

## Troubleshooting

If Gradle fails with an invalid `JAVA_HOME`, point it at JDK 17 for the current shell:

```powershell
$env:JAVA_HOME = "C:\path\to\jdk-17"
$env:Path = "$env:JAVA_HOME\bin;$env:Path"
```

If CMake fails because `Externals\libadrenotools` is empty or missing `CMakeLists.txt`, populate that external dependency before building. In the local build, this was repaired by cloning `libadrenotools` and its nested submodule:

```powershell
git clone --depth 1 https://github.com/bylaws/libadrenotools.git Externals/libadrenotools
git -C Externals/libadrenotools submodule update --init --recursive
```

If Kotlin fails on missing Quest settings constants, check that the Android build defines `BuildConfig.IS_QUEST` and exposes `FloatSetting.GFX_VR_UNITS_PER_METER`.

If native compilation fails on Android achievement calls, check that achievement JNI calls are guarded by `USE_RETRO_ACHIEVEMENTS`.
