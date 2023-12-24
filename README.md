# Interpreters

### Mac OS Toolchain

- [MacPorts](https://www.macports.org)
- clang-17
- cmake

After installing MacPorts:

    > sudo port install clang-17 cmake
    > sudo port select clang mp-clang-17

### Building

    cd <project-dir>
    > makedir .bld
    > cd .bld
    > cmake ..
    > make -j

### Running

    > make && blip/blip

### Running Tests

    > make && core/core-test
