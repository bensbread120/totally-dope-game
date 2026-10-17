# totally-dope-game

# It is totally dope
# Indeed the dopeness here is immaculate

A top-down adventure prototype written in C++20 with [SDL3](https://libsdl.org), [SDL3_image](https://github.com/libsdl-org/SDL_image) and [glm](https://github.com/g-truc/glm).

**Controls:** arrow keys to move, `1` to toggle debug view (colliders and object counts).

## Prerequisites

You need a C++20 compiler and **CMake 3.24 or newer**. You don't need to install the libraries yourself (see [Getting the libraries](#getting-the-libraries)).

### Windows

Install [Visual Studio 2022 or newer](https://visualstudio.microsoft.com/) (Community is fine) with the **Desktop development with C++** workload. That gives you the MSVC compiler, CMake and Ninja.

If you'd rather work only in the terminal, the free [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio) with the same workload is enough. Install [CMake](https://cmake.org/download/) separately if `cmake --version` doesn't work in your terminal.

### Linux

You need GCC 13+ (or Clang 17+) for `std::format`, and CMake 3.24+. Ubuntu 24.04 or newer meets both, but Ubuntu 22.04 is too old.

SDL is built from source, so it also needs the development headers for your window system and audio. On Ubuntu/Debian:

```bash
sudo apt install build-essential cmake ninja-build pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev \
  libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev libdecor-0-dev \
  libegl1-mesa-dev libgl1-mesa-dev libgles2-mesa-dev libdrm-dev libgbm-dev \
  libasound2-dev libpulse-dev libpipewire-0.3-dev libdbus-1-dev libudev-dev
```

For Fedora, Arch and others, see the package lists in SDL's [README-linux.md](https://github.com/libsdl-org/SDL/blob/main/docs/README-linux.md#build-dependencies).

## Getting the libraries

**By default you don't have to do anything.** The first time you configure, CMake downloads SDL3 3.4.14, SDL3_image 3.4.4 and glm 1.0.3 into the build folder and builds them along with the game. That first configure and build takes a few minutes. Later builds reuse the result.

If CMake can find an installed copy of a library first, it uses that instead and skips the download. Two cases where that's useful:

- **Prebuilt SDL on Windows.** This saves the time spent compiling SDL. Download `SDL3-devel-3.4.14-VC.zip` from the [SDL releases page](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.14) and `SDL3_image-devel-3.4.4-VC.zip` from the [SDL_image releases page](https://github.com/libsdl-org/SDL_image/releases/tag/release-3.4.4). Extract them anywhere, for example `C:\dev`, then point CMake at them when configuring:

  ```powershell
  cmake -S . -B build -D CMAKE_PREFIX_PATH="C:/dev/SDL3-3.4.14;C:/dev/SDL3_image-3.4.4"
  ```

  glm is small and header-only, so it's simplest to let it download.

- **Distro packages on Linux.** If your distribution packages SDL3 and SDL3_image, CMake picks them up automatically once they're installed.

To ignore any installed copies and always download, add `-D FETCHCONTENT_TRY_FIND_PACKAGE_MODE=NEVER` to the configure command.

## Building from the terminal

Building with CMake takes two steps:

1. **Configure.** CMake reads `CMakeLists.txt` and generates a project for your platform's build tool (a Visual Studio solution, Ninja files, or Makefiles) in the `build` folder.
2. **Build.** CMake runs that build tool.

Run these from the repository root.

### Windows (PowerShell, cmd, or Git Bash)

```powershell
cmake -S . -B build
cmake --build build
.\build\sdl3-demo\Debug\sdl3-demo.exe
```

This generates a Visual Studio solution. A solution holds both Debug and Release configurations, like a C# solution does, so you pick the configuration at build time: `cmake --build build --config Release` puts the exe in `build\sdl3-demo\Release\`.

### Linux

```bash
cmake -S . -B build -G Ninja -D CMAKE_BUILD_TYPE=Debug
cmake --build build
./build/sdl3-demo/sdl3-demo
```

Ninja and Makefiles only hold one configuration per build folder, so you pick it at configure time with `CMAKE_BUILD_TYPE` (`Debug` or `Release`). To have both, use a second build folder. You can drop `-G Ninja` to use Makefiles instead.

The build copies `assets/` (and on Windows, the SDL DLLs) next to the executable, so you can launch it from any folder.

## Building in Visual Studio

1. **File → Open → Folder…** and select the repository root. Visual Studio detects `CMakeLists.txt` and configures automatically.
2. In the configuration dropdown on the toolbar, pick **x64 Debug**.
3. In the startup item dropdown, pick **sdl3-demo.exe** and press F5.

To use prebuilt libraries here, add a `CMAKE_PREFIX_PATH` cache variable to a preset in `CMakeUserPresets.json`. See the `x64-debug-user` preset for an example.

## Troubleshooting

- **`validity error : No declaration for attribute ...` while building SDL on Linux.** This is harmless output from SDL's Wayland code generator. The build carries on.
- **Changed `CMAKE_PREFIX_PATH` but CMake still uses the old libraries.** CMake caches what it found. Delete the `build` folder and configure again.
