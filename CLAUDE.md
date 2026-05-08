# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Components

Three subprojects, each with its own README for details:

- **`core/`** — header-only `INTERFACE` library; shared compiler primitives (Token, LexerInterface, SourceLocation, CompilerException). Always built.
- **`bust/`** — the active language. Rust-flavored, HM type inference, monomorphization, LLVM IR codegen.
- **`blip/`** — older s-expression interpreter, kept for reference. **Opt-in via `BUILD_BLIP=ON`** — disabled by default.

**Default focus is bust.** Don't touch blip unless explicitly asked.

## Build Commands

Requires a local `CMakeUserPresets.json` (gitignored) — see README.md for the template.

```bash
cmake --preset mp                          # lean: only core + bust (everyday)
cmake --build --preset mp
ctest --preset mp
ctest --preset mp -R bust.type_checker     # filter by regex
ctest --preset mp -N                       # list all registered suites
```

`mp-full` (`build-full/`) also builds blip and is what `.githooks/pre-push` runs, so blip still gets verified before any push. Release variants: `mp-release`, `mp-release-full`.

To run the bust CLI:

```bash
./build/bust/bust            # interactive
./build/bust/bust file.bu    # run a script
```

## Testing

DocTest (assertions) + RapidCheck (property-based). Each language has its own test binary (`bust-test`, `blip-test`); each `TEST_SUITE` is registered individually via `add_test`, so topical names like `bust.parser.core` and `bust.type_checker.basics` show up as separate ctest targets. Source files for bust tests live in `bust/test/`; blip tests in `blip/test/`.

## Key Design Decisions

- `core` is header-only (CMake `INTERFACE` library) — all templates, no compiled sources.
- `Token<TT>` is parameterized on the token-type enum, so each language gets full `enum class` exhaustiveness warnings in switches.
- Lexers are hidden behind factory functions (`blip::make_lexer()`, `bust::make_lexer()`); implementations live in anonymous namespaces.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` is set globally; clangd may need pointing at `build/compile_commands.json` vs `build-full/compile_commands.json` depending on which preset was last configured.
- Dependencies (DocTest, RapidCheck) are fetched via `FetchContent` through custom Find modules in `cmake/Find*.cmake`.
- `cmake/Platform.cmake` patches the libc++ path for MacPorts clang 16–18. Not needed for clang 21+, kept for compatibility.
