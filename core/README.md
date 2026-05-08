# core

Header-only `INTERFACE` library — shared compiler infrastructure used by both
`bust` and `blip`. Always built; always included.

Namespace: `core::`

## What's in here

All public types live in `core/inc/`:

- `token.hpp` — `Token<TT>` / `TokenOf<TT, V>` parameterized on a token-type
  enum, so each language gets full `enum class` exhaustiveness in switches.
- `lexer_interface.hpp` — `LexerInterface<TT>`, the abstract lexer contract.
- `source_location.hpp` — filename / line / column tracking for diagnostics.
- `exceptions.hpp` — `CompilerException` carrying a phase name + source
  location.
- `scope_guard.hpp` — RAII helper.
- `hash_combine.hpp` — small hashing utility.

There's no compiled output: `add_library(core INTERFACE)` only propagates
include paths and warning flags.

## Why it's a separate target

Pulling these into a shared target prevents drift between bust and blip and
gives each language a single canonical lexer/token contract to implement.
