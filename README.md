
# Command & Conquer

This repository includes source code for Command & Conquer. This release provides support to the [Steam Workshop](https://steamcommunity.com/workshop/browse/?appid=2229830) for the game.


## Dependencies

If you wish to rebuild the source code and tools successfully you will need to find or write new replacements (or remove the code using them entirely) for the following libraries;

- DirectX 5 SDK
- Greenleaf Communications Library (GCL)
- Human Machine Interface (HMI) “Sound Operating System” (SOS)

## SDL3/CMake Android build

This branch now includes a repo-owned Android packaging path for the SDL3/CMake port.

Use the Arch-style SDK/NDK layout directly:

```sh
source /etc/profile.d/android-ndk.sh
export ANDROID_HOME=/opt/android-sdk
export ANDROID_SDK_ROOT=/opt/android-sdk

cmake -S . -B build-android \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26

cmake --build build-android --target tiberian-dawn-apk -j
```

That produces:

- `build-android/libtiberian-dawn.so`
- `build-android/tiberian-dawn-code.apk`
- `build-android/tiberian-dawn-data.apk`

For the common build/deploy loop on a connected device, run:

```sh
./launcha.sh
```

The launcher script bootstraps the SDK/NDK environment from the Arch package locations, configures `build-android` if needed, builds `tiberian-dawn-apk`, installs the code/data APKs with direct `adb` commands, and starts `com.ea.tiberiandawn/.TiberianDawnActivity`.


## Touch controls

- **Single tap / drag during gameplay:** left click and drag-select.
- **Two-finger tap during gameplay:** right click, which also gives the usual deselect behavior.
- **Two-finger drag during gameplay:** pans the battlefield by driving the existing edge-scroll behavior.
- **Double tap during movies:** cancels/skips the current movie, like `Esc` or `Return`.


## Compiling (Win32 Only)

The current state of the source code does not fully compile and does not contain the core engine libraries (you can find these in the [Red Alert](https://github.com/electronicarts/CnC_Red_Alert) code repository) and will require some effort to restore it. If you wish to restore the original build environment, the following tools are required;

- Watcom C/C++ (v10.6) for C/C++ source files
- Borland Turbo Assembler (TASM v4.0) for assembly files
- Microsoft Micro Assembler (MASM v6.11d) for assembly files

To use the compiled binaries, you must own the game. The C&C Ultimate Collection is available for purchase on [EA App](https://www.ea.com/en-gb/games/command-and-conquer/command-and-conquer-the-ultimate-collection/buy/pc) or [Steam](https://store.steampowered.com/bundle/39394/Command__Conquer_The_Ultimate_Collection/).


## Contributing

This repository will not be accepting contributions (pull requests, issues, etc). If you wish to create changes to the source code and encourage collaboration, please create a fork of the repository under your GitHub user/organization space.


## Support

This repository is for preservation purposes only and is archived without support. 


## License

This repository and its contents are licensed under the GPL v3 license, with additional terms applied. Please see [LICENSE.md](LICENSE.md) for details.
