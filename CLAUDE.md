# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Requires a local `CMakeUserPresets.json` (gitignored) — see README.md for the template.

```bash
cmake --preset mp                  # configure (run once, or after CMake changes)
cmake --build --preset mp          # build
ctest --preset mp                  # run all tests
ctest --preset mp -R core.lexer    # run a single test suite by name
```

Test suite names: `core.token`, `core.lexer`.

To run the blip REPL directly after building:

```bash
./build/blip/blip          # interactive
./build/blip/blip file.bl  # run a script
```

## Architecture

Two components:

**`core/` — static library (`libcore`), namespace `sc::`**
The foundational compiler infrastructure. Provides:
- `Token` / `TokenOf<T>` — typed tokens carrying a `SourceLocation`
- `LexerInterface` — abstract interface with `get_next_token()`
- `CoreLexer` — full lexer implementation, hidden behind a factory function `sc::make_lexer()` in `sc/sc.hpp`. Accepts a caller-supplied keyword map so it's reusable across languages.
- `CompilerException` — exception type that includes source location

**`blip/` — executable + static library, namespace `blip::`**
The blip language interpreter, built on top of `core`. Currently only lexes — `Blip::rep()` tokenizes input and dumps tokens. Parsing and evaluation are not yet implemented.

The `blip` keywords (`IF`, `WHILE`, `SET`, `BEGIN`, `PRINT`, `DEFINE`) suggest an s-expression syntax. See `PARSER_PLAN.md` for the planned parser implementation.

## Testing

Tests use **DocTest** (assertions) + **RapidCheck** (property-based). Test infrastructure lives in `core/test/inc/core_test_lib.hpp`, which provides `rc::Arbitrary` implementations for `SourceLocation` and `Token`.

Individual test suites are invoked via the `-ts=` flag on the `core-test` binary, which is what the named CTest targets wrap.

## Key Design Decisions

- `CoreLexer` is in an anonymous namespace — it's only accessible via `sc::make_lexer()`, keeping the implementation hidden.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` is set globally, so `build/compile_commands.json` is always generated for clangd.
- Dependencies (DocTest, RapidCheck) are fetched via `FetchContent` through custom Find modules in `cmake/FindDocTest.cmake` and `cmake/FindRapidCheck.cmake`.
- `cmake/Platform.cmake` patches the libc++ path for MacPorts clang versions 16–18. Not needed for clang 21+, but kept for compatibility.
