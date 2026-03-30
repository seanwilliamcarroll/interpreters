# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Requires a local `CMakeUserPresets.json` (gitignored) — see README.md for the template.

```bash
cmake --preset mp                  # configure (run once, or after CMake changes)
cmake --build --preset mp          # build
ctest --preset mp                  # run all tests
ctest --preset mp -R blip.lexer    # run a single test suite by name
```

Test suite names: `blip.token`, `blip.lexer`, `blip.parser`, `blip.ast_printer`.

To run the blip REPL directly after building:

```bash
./build/blip/blip          # interactive
./build/blip/blip file.bl  # run a script
```

## Architecture

Two components:

**`core/` — header-only INTERFACE library, namespace `core::`**
Minimal compiler infrastructure, all templates:
- `Token<TT>` / `TokenOf<TT, V>` — token class templates parameterized on a token type enum
- `LexerInterface<TT>` — abstract lexer interface, returns `Token<TT>`
- `SourceLocation` — filename, line, column tracking
- `CompilerException` — exception type with phase name and source location

**`blip/` — executable + static library, namespace `blip::`**
The blip language interpreter. Defines:
- `enum class TokenType` — all token types (structural, literals, keywords) in one place
- Type aliases: `Token`, `TokenString`, `TokenInt`, etc. instantiating core templates
- Lexer implementation (hidden behind `blip::make_lexer()` factory)
- Parser (recursive descent for s-expressions)
- AST nodes (all in `blip::` — literals, identifiers, special forms)
- Visitor pattern (`AstVisitor`) and `AstPrinter`

## Testing

Tests use **DocTest** (assertions) + **RapidCheck** (property-based). All tests live in `blip/test/`.

Individual test suites are invoked via the `-ts=` flag on the `blip-test` binary, which is what the named CTest targets wrap.

## Key Design Decisions

- The lexer is in an anonymous namespace — only accessible via `blip::make_lexer()`, keeping the implementation hidden.
- Core is header-only (CMake INTERFACE library) — all templates, no compiled sources.
- `Token<TT>` is parameterized on the token type, so blip gets full `enum class` type safety with exhaustiveness warnings in switch statements.
- `CMAKE_EXPORT_COMPILE_COMMANDS=ON` is set globally, so `build/compile_commands.json` is always generated for clangd.
- Dependencies (DocTest, RapidCheck) are fetched via `FetchContent` through custom Find modules in `cmake/FindDocTest.cmake` and `cmake/FindRapidCheck.cmake`.
- `cmake/Platform.cmake` patches the libc++ path for MacPorts clang versions 16–18. Not needed for clang 21+, but kept for compatibility.
