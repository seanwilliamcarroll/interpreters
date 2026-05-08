# Interpreters

### Mac OS Toolchain

- [MacPorts](https://www.macports.org)
- clang-21 (or latest available)
- cmake

After installing MacPorts:

    sudo port install clang-21 cmake
    sudo port select clang mp-clang-21

### Build Configuration (CMake Presets)

This project uses CMake presets. `CMakePresets.json` (committed) defines shared
baseline settings. Machine-specific configuration lives in `CMakeUserPresets.json`,
which is gitignored and must be created locally.

The build is split into two interpreters:

- **`bust`** (always built): the actively-developed Rust-like language.
- **`blip`** (opt-in via `BUILD_BLIP=ON`): an earlier s-expression interpreter
  kept around for reference. Disabled by default to keep the everyday
  build/test cycle quick.

Each build mode comes in two variants:

| Preset            | Build type | Blip | Build dir            |
|-------------------|------------|------|----------------------|
| `mp`              | Debug+ASan | off  | `build/`             |
| `mp-full`         | Debug+ASan | on   | `build-full/`        |
| `mp-release`      | Release    | off  | `build-release/`     |
| `mp-release-full` | Release    | on   | `build-release-full/`|

Lean presets are for day-to-day work — only core + bust. Full presets also
build blip and are what the pre-push hook uses to verify nothing else broke.
Each preset has its own build directory so they coexist without trampling
caches.

Create `CMakeUserPresets.json` in the project root:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "mp-base",
      "hidden": true,
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_C_COMPILER": "/opt/local/libexec/llvm-21/bin/clang",
        "CMAKE_CXX_COMPILER": "/opt/local/libexec/llvm-21/bin/clang++",
        "CMAKE_OSX_SYSROOT": "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
      }
    },
    {
      "name": "mp",
      "displayName": "MacPorts Debug+ASan (lean: core + bust)",
      "inherits": "mp-base",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "ENABLE_ASAN": "ON",
        "BUILD_BLIP": "OFF"
      }
    },
    {
      "name": "mp-full",
      "displayName": "MacPorts Debug+ASan (full: core + bust + blip)",
      "inherits": "mp-base",
      "binaryDir": "${sourceDir}/build-full",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "ENABLE_ASAN": "ON",
        "BUILD_BLIP": "ON"
      }
    },
    {
      "name": "mp-release",
      "displayName": "MacPorts Release (lean: core + bust)",
      "inherits": "mp-base",
      "binaryDir": "${sourceDir}/build-release",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "ENABLE_ASAN": "OFF",
        "BUILD_BLIP": "OFF"
      }
    },
    {
      "name": "mp-release-full",
      "displayName": "MacPorts Release (full: core + bust + blip)",
      "inherits": "mp-base",
      "binaryDir": "${sourceDir}/build-release-full",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "ENABLE_ASAN": "OFF",
        "BUILD_BLIP": "ON"
      }
    }
  ],
  "buildPresets": [
    { "name": "mp",              "configurePreset": "mp" },
    { "name": "mp-full",         "configurePreset": "mp-full" },
    { "name": "mp-release",      "configurePreset": "mp-release" },
    { "name": "mp-release-full", "configurePreset": "mp-release-full" }
  ],
  "testPresets": [
    { "name": "mp",              "configurePreset": "mp",              "output": { "outputOnFailure": true } },
    { "name": "mp-full",         "configurePreset": "mp-full",         "output": { "outputOnFailure": true } },
    { "name": "mp-release",      "configurePreset": "mp-release",      "output": { "outputOnFailure": true } },
    { "name": "mp-release-full", "configurePreset": "mp-release-full", "output": { "outputOnFailure": true } }
  ]
}
```

Adjust the clang version and SDK path to match your system. The SDK path can be
found with `xcrun --show-sdk-path`.

### Git Hooks

This project includes shared git hooks in `.githooks/`. To use them, configure
your local clone to point at that directory:

    git config core.hooksPath .githooks

Currently includes a **pre-commit** hook that runs `clang-format` on staged
C/C++ files. If `clang-format` is not installed, the hook is silently skipped.

### Building

    cmake --preset mp           # lean: core + bust
    cmake --build --preset mp

    cmake --preset mp-full      # full: also builds blip
    cmake --build --preset mp-full

### Running

    ./build/bust/bust           # bust REPL / script runner
    ./build-full/blip/blip      # blip REPL (only available in full build)

### Running Tests

    ctest --preset mp           # core + bust tests
    ctest --preset mp-full      # also runs blip tests
