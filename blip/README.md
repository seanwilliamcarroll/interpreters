# blip

Earlier minimal s-expression interpreter, kept as a reference / playground.
**Not actively developed** — opt in by configuring with `BUILD_BLIP=ON` (the
`mp-full` / `mp-release-full` presets).

Namespace: `blip::`

## What it is

A tree-walking interpreter for a tiny Lisp-ish language: lexer, recursive-
descent s-expression parser, AST with the visitor pattern, simple type
checker, and an evaluator backed by an `Environment`. Intended as a smaller,
independent design from `bust`.

## Layout

| Path        | Role                                                           |
|-------------|----------------------------------------------------------------|
| `inc/`      | Public headers (lexer, parser, AST, type checker, evaluator).  |
| `src/`      | Implementation + CLI entry.                                    |
| `programs/` | Example `.bl` programs.                                        |
| `test/`     | DocTest + RapidCheck unit tests.                               |

The lexer implementation is hidden behind `blip::make_lexer()` — the
implementation lives in an anonymous namespace.

## Build & run

Blip is excluded by the default lean preset. Use the full preset:

    cmake --preset mp-full
    cmake --build --preset mp-full
    ./build-full/blip/blip                   # interactive REPL
    ./build-full/blip/blip path/to/file.bl   # run a script

## Tests

    ctest --preset mp-full -R '^blip\.'

Suites: `blip.token`, `blip.lexer`, `blip.parser`, `blip.ast_printer`,
`blip.environment`, `blip.evaluator`, `blip.builtins`, `blip.type_checker`.
