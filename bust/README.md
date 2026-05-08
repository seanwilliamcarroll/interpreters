# bust

The actively-developed language in this repo: a small Rust-flavored language
with Hindley–Milner type inference, monomorphization, and LLVM IR codegen.

Namespace: `bust::`

## Pipeline

Source → Lexer → Parser → AST → Type Checker → HIR → Monomorphizer → ZIR
Lowerer → ZIR → Codegen → LLVM IR.

Each phase is reachable via `bust::run_pipeline(...)` (see
`inc/pipeline.hpp`); `Bust` in `inc/bust.hpp` is the top-level driver used by
the CLI.

## Layout

| Path             | Role                                                      |
|------------------|-----------------------------------------------------------|
| `inc/`, `src/`   | Public headers and the `bust` CLI driver.                 |
| `ast/`           | Parser AST nodes + dumper.                                |
| `hir/`           | Type-checked IR — types, type unifier, type checker.      |
| `mono/`          | Monomorphization (specializing polymorphic definitions).  |
| `zir/`           | Lower-level IR consumed by codegen.                       |
| `codegen/`       | LLVM IR emission.                                         |
| `std/`           | Prelude written in bust itself.                           |
| `programs/`      | Sample `.bu` programs (also driven by the integration test). |
| `test/`          | Unit + integration tests.                                 |
| `grammar.md`     | EBNF-ish grammar reference.                               |
| `testing.md`     | Notes on the testing strategy.                            |

## Build & run

    cmake --preset mp
    cmake --build --preset mp
    ./build/bust/bust path/to/program.bu     # run a script
    ./build/bust/bust                        # interactive

`bust --help` lists `--dump-*` flags for inspecting each pipeline stage.

## Tests

    ctest --preset mp                        # everything
    ctest --preset mp -R bust.type_checker   # one suite (regex match)
    ctest --preset mp -R bust.codegen        # all codegen sub-suites

Test suites are split topically — see `bust.parser.*`, `bust.type_checker.*`,
`bust.codegen.*`, `bust.integration`, etc.
