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

Create `CMakeUserPresets.json` in the project root, inheriting from `base`:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "macports",
      "displayName": "MacPorts Clang",
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_C_COMPILER": "/opt/local/libexec/llvm-21/bin/clang",
        "CMAKE_CXX_COMPILER": "/opt/local/libexec/llvm-21/bin/clang++",
        "CMAKE_OSX_SYSROOT": "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
      }
    }
  ],
  "buildPresets": [
    { "name": "macports", "configurePreset": "macports" }
  ],
  "testPresets": [
    { "name": "macports", "configurePreset": "macports", "output": { "outputOnFailure": true } }
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

    cmake --preset macports
    cmake --build --preset macports

### Running

    ./build/blip/blip

### Running Tests

    ctest --preset macports
