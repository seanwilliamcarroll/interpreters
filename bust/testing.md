# Bust Testing Strategy

## Layers

The bust test suite has three layers, each living in `bust/test/src/`:

1. **Unit** — narrow per-module tests. Each compiler stage owns a test
   binary section: `lexer_test`, `parser_test`, `type_checker_test`,
   `type_unifier_test`, `monomorpher_test`, `zir_lowerer_test`,
   `validate_main_test`, `name_mangler_test`, `free_variable_collector_test`.
   These verify each component in isolation, with hand-built fixtures and
   targeted error cases.

2. **Codegen** — feature-grouped tests that drive the full pipeline (parse →
   typecheck → monomorph → ZIR → LLVM IR) against inline bust source and
   either inspect IR shape or execute via `lli` and assert exit code /
   stdout. Lives in `codegen_expressions_test`, `codegen_functions_test`,
   `codegen_lambdas_test`, `codegen_tuples_test`, `codegen_side_effects_test`,
   `codegen_loops_test`. Source is inline string literals; expectations
   are `CHECK_RUN(src, exit)` / `CHECK_RUN_OUTPUT(src, exit, stdout)`.

3. **Integration** — full programs loaded from disk, exercising
   *combinations* of features. Lives in `integration_test.cpp` (suite
   `bust.integration`), with programs in `bust/programs/` (flat — no
   subdirectory). Each program is a complete `.bu` file with expected
   exit code and stdout encoded in header comments. The test driver
   runs the entire pipeline, attaches dumps from every successful
   stage, and re-raises any captured error so doctest reports the
   failure with all available context.

## Pipeline + Dumps

Every integration test runs the source through the same pipeline as the
real `bust` driver: **source → AST → HIR → ZIR → LLVM IR → run via lli**.

Each stage has a dumper:

| Stage   | Dumper                        |
|---------|-------------------------------|
| AST     | `ast::Dumper::dump(Program)`  |
| HIR     | `hir::Dumper::dump(Program)`  |
| MONO    | `mono::Dumper::dump(Program)` |
| ZIR     | `zir::Dumper::dump(Program)`  |
| LLVM IR | `CodeGen()(program)` (text)   |

The `run_pipeline` helper runs each stage in a try/catch, populates the
dump on success, captures the exception on failure, and **never throws**.
Source is always populated. The test then attaches every populated dump
via `INFO()` (lazy — printed only on test failure) and re-raises the
captured error if any.

This means a failing integration test always shows you, in order:
- the source program,
- every IR dump produced before the failure,
- the actual error.

If type-checking throws, you see source + AST. If codegen throws, you
see source + AST + HIR + ZIR. If the program runs but exits with the
wrong code, you see all dumps plus exit code/stdout diff.

## Per-program Manifest (Header Comments)

Each integration program declares its expectations in a header comment:

```bust
// EXPECT_EXIT: 42
// EXPECT_STDOUT: "012"
fn main() -> i64 {
  // ...
}
```

Encoding:
- `EXPECT_EXIT: <int>` — required.
- `EXPECT_STDOUT: "<text>"` — optional. Default empty. Standard escapes
  (`\n`, `\t`, `\\`, `\"`) recognized.
- Lines with neither prefix are ignored, so freeform documentation can
  precede the program.

A program with no `EXPECT_EXIT` line is rejected by the loader with a
clear error — silent skips are worse than visible failures.

## Coverage Goal

Every language feature listed below should appear in at least one
integration program. The matrix at the bottom of this doc tracks which
program covers which features. One program (`everything.bu`) is meant
to touch as many features as practical in a single coherent program;
the others are focused two-or-three-feature combinations.

---

## Language Features

The taxonomy below is the source-of-truth for the matrix. Numbering is
stable; add new features at the end.

### Lexical / Literals
- **F1** — Integer literals (decimal)
- **F2** — Char literals with escapes (`\n`, `\t`, `\xHH`, etc.)
- **F3** — Bool literals (`true`, `false`)
- **F4** — Unit literal `()`
- **F5** — Line comments (`//`)
- **F6** — Block comments (`/* ... */`), including nested

### Types
- **F7** — Primitive types: `i8`, `i32`, `i64`
- **F8** — `bool`
- **F9** — `char`
- **F10** — `()` (unit type)
- **F11** — Function types `fn(T, ...) -> U`
- **F12** — Tuple types `(T, U, ...)` and 1-tuple `(T,)`
- **F13** — Type annotations on let / parameters

### Bindings
- **F14** — Immutable `let`
- **F15** — Mutable `let mut`
- **F16** — Shadowing
- **F17** — Assignment to mutable place

### Operators
- **F18** — Arithmetic: `+ - * / %`
- **F19** — Unary: `-`, `!`
- **F20** — Comparison: `== != < > <= >=`
- **F21** — Logical short-circuit: `&&`, `||`
- **F22** — Precedence + parentheses

### Casts
- **F23** — `as` cast: numeric narrowing / widening, bool → int,
  char ↔ i8/i64

### Control Flow
- **F24** — Block expression (with/without trailing expression)
- **F25** — `if` / `else` as expression returning a value
- **F26** — `if`-then with no else (unit-typed statement form)
- **F27** — `while` loop (unit-typed expression)
- **F28** — `break` (inside loop body; type Never)
- **F29** — `return <expr>`
- **F30** — Naked `return` (sugar for `return ()`)

### Functions
- **F31** — Top-level `fn` definition
- **F32** — Function call
- **F33** — Recursion (direct self-call)
- **F34** — Multiple parameters
- **F35** — Implicit return type → `()` when `-> T` omitted

### Lambdas / Closures
- **F36** — Lambda expression `|args| body`
- **F37** — Lambda with explicit return type `|args| -> T body`
- **F38** — Lambda with inferred parameter type
- **F39** — Closure capturing immutable variable
- **F40** — Closure capturing mutable variable
- **F41** — Calling a lambda multiple times

### Tuples
- **F42** — Tuple literal `(a, b, ...)` (≥ 2 elements)
- **F43** — 1-tuple `(a,)`
- **F44** — Tuple projection `.N`
- **F45** — Tuple as function parameter / return type

### Polymorphism (HM + Monomorphization)
- **F46** — Hindley-Milner inference of let / parameter types
- **F47** — Polymorphic function instantiated at multiple concrete types

### FFI
- **F48** — `extern fn` declaration
- **F49** — Calling an extern (e.g., `putchar`)

### Loop expressions (typed-result and continue)
- **F50** — `loop { ... }` infinite-or-break loop expression. Result type
  is the unified type of its `break <expr>` payloads, or `Never` when
  the body has no break path out.
- **F51** — `break <expr>` carrying a typed payload. Valid only inside
  `loop` (a `while` body's `break` must be unit-typed). All payloads of
  break sites in the same `loop` must unify.
- **F52** — `continue` — skip the rest of the body and resume at the
  loop's re-entry point (condition for `while`, body label for `loop`).

### Semantic Invariants (cross-cutting; verified by error-path tests)
- **I1** — `main` exists and returns `i64`
- **I2** — `break` only inside loop bodies (type-checker enforced)
- **I3** — `let mut` required for assignment to a place
- **I4** — While body must be unit-typed; condition must be bool
- **I5** — Lambda body type must match declared / inferred return type
- **I6** — `continue` only inside loop bodies (type-checker enforced)
- **I7** — `break <expr>` (non-unit payload) only valid inside `loop`,
  not `while`

---

## Existing Integration Programs

These programs live in `bust/programs/` and are wired into the
`bust.integration` suite. Their feature columns reference the F-numbers
from the taxonomy above.

### Smoke / canonical examples

| Program                  | Features Covered                              | Notes                                                    |
|--------------------------|-----------------------------------------------|----------------------------------------------------------|
| `hello_world.bu`         | F2, F23, F31, F32, F35, F48, F49              | Canonical extern + putchar program.                      |
| `fibonacci.bu`           | F1, F18, F20, F25, F31, F33, F34              | Doubly-recursive fib(10) = 55.                           |
| `lambda.bu`              | F11, F31, F32, F34, F36, F37, F39             | Higher-order `apply` taking `fn(i64) -> i64`.            |
| `polymorphic.bu`         | F25, F29, F36, F38, F46, F47                  | Identity lambda used at both `bool` and `i64`.           |
| `short_circuit.bu`       | F1, F8, F13, F14, F18, F20, F21, F25          | `&&` / `\|\|` chains; commented for IR inspection.       |
| `side_effect_return.bu`  | F2, F23, F30, F36, F49                        | Lambda + naked `return` + dead code; prints `Hi\n`.      |

### Loop combinations

| Program                       | Features Covered                                   | Notes                                                                        |
|-------------------------------|----------------------------------------------------|------------------------------------------------------------------------------|
| `loop_helper_lambda.bu`       | F2, F15, F17, F20, F23, F27, F36, F38, F39, F49    | Closure capturing immutable `banner` called inside `while` loop.             |
| `loop_with_recursion.bu`      | F1, F15, F17, F18, F20, F25, F27, F31, F33, F34    | `sum_to(i)` invoked each iteration; total = 1 + 3 + 6 = 10.                  |
| `nested_loops_break.bu`       | F1, F15, F17, F20, F23, F25, F27, F28, F49         | Inner `break` exits only innermost loop; outputs "OiiOiiOii".                |
| `lambda_with_loop.bu`         | F1, F3, F15, F17, F20, F23, F25, F27, F28, F36, F49| Lambda body contains `while` + `break`; `loop_stack` per-lambda.             |
| `lambda_return_in_loop.bu`    | F2, F15, F17, F20, F23, F27, F30, F36, F49         | Lambda's `return` exits only the lambda; outer loop continues.               |
| `tuple_loop.bu`               | F1, F15, F17, F18, F20, F27, F42, F44              | 2-tuple reassigned each iteration; final `t.1 == 3`.                         |
| `poly_in_loop.bu`             | F1, F3, F15, F17, F20, F25, F27, F36, F38, F46, F47| Polymorphic `id` instantiated at i64 (in loop) and bool (after loop).        |
| `cast_chain_loop.bu`          | F1, F2, F7, F9, F13, F15, F17, F20, F23, F27, F49  | i8 counter, widening casts to i32/i64 in condition and body.                 |
| `short_circuit_in_loop.bu`    | F1, F2, F3, F15, F17, F20, F21, F23, F25, F27, F28, F49 | `&&` / `\|\|` mixed in a break-guard; SC proves no eager eval.          |

### loop / break-with-value / continue

| Program                       | Features Covered                                          | Notes                                                                                  |
|-------------------------------|-----------------------------------------------------------|----------------------------------------------------------------------------------------|
| `loop_break_value.bu`         | F1, F15, F17, F18, F20, F50, F51                          | Counter walks until i*7 == 42; `break i*7` exits with the answer.                      |
| `continue_skip_evens.bu`      | F1, F15, F17, F18, F20, F23, F26, F27, F49, F52           | `while` + `continue` pattern; prints odd values 1..9.                                  |
| `loop_break_in_lambda.bu`     | F1, F11, F13, F15, F17, F20, F32, F36, F37, F50, F51      | Lambda body uses `loop` + `break <val>`; verifies per-lambda loop_stack.               |
| `loop_break_tuple.bu`         | F1, F14, F15, F17, F18, F20, F42, F44, F50, F51           | `break (i, i*i)` carries a 2-tuple; combines tuple alloca with loop-result alloca.     |

### Catch-all

| Program          | Features Covered                                                                                                 | Notes                                                                |
|------------------|------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------|
| `everything.bu`  | F1–F4, F7–F9, F11, F13–F23, F25–F39, F41–F49                                                                     | Single coherent program touching most features. Output `Y5abc\n.` Does not yet cover F50–F52 — follow-up. |

### Negative tests (typecheck rejects)

| Program                          | Notes                                                                |
|----------------------------------|----------------------------------------------------------------------|
| `bad_poly.bu`                    | `EXPECT_FAIL: typecheck`. `\|x\|{x+x}(true)` — Numeric constraint.   |
| `bad_poly_2.bu`                  | `EXPECT_FAIL: typecheck`. `\|x,y\|{x+y}(1, true)` — unification.     |
| `bad_continue_outside_loop.bu`   | I6. `continue;` at function top.                                     |
| `bad_break_value_in_while.bu`    | I7. `while true { break 5; }` — payload not allowed on while-break.  |

## Forward-looking Matrix (deferred)

Programs that are blocked on features not yet implemented — kept here
as a backlog. They'll be added when the prerequisite feature lands.

| Program                       | Features Covered                                                           | What It Demonstrates                                                                                  |
|-------------------------------|----------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------|
| `loop_closure_counter.bu`     | F1, F15, F17, F18, F20, F27, F28, F36, F40, F49                            | While loop driven by a closure that mutates a captured counter; break exits when threshold reached.   |
| `loop_with_recursion.bu`      | F1, F18, F27, F29, F33, F34, F49                                           | Recursive function called from inside a while body; combined exit via `return`.                       |
| `nested_loops_break.bu`       | F1, F15, F17, F20, F27, F28, F49                                           | Nested `while`s where inner `break` only exits the innermost loop.                                    |
| `lambda_with_loop.bu`         | F15, F17, F20, F27, F28, F36, F49                                          | A lambda whose body contains a `while` + `break`; verifies per-lambda loop_stack scoping.             |
| `lambda_return_in_loop.bu`    | F15, F17, F27, F29, F30, F36, F39, F49                                     | Lambda called inside a `while`; the lambda's `return` exits the lambda only, the loop continues.      |
| `tuple_loop.bu`               | F1, F15, F17, F18, F20, F27, F42, F44, F45                                 | Pair `(value, count)` mutated across loop iterations; tuple projection drives both branches.          |
| `poly_in_loop.bu`             | F1, F8, F15, F17, F23, F27, F46, F47, F49                                  | Polymorphic identity/helper instantiated at multiple types inside the same surrounding code.          |
| `closure_returning_closure.bu`| F11, F32, F36, F37, F39, F40, F41                                          | A function that returns a lambda; outer captures shape the inner closure.                             |
| `cast_chain_loop.bu`          | F1, F2, F7, F9, F15, F17, F18, F20, F23, F27, F49                          | Casts driving comparisons in a loop condition (i64 ↔ i32 ↔ char), feeding extern call.                |
| `short_circuit_in_loop.bu`    | F3, F15, F17, F20, F21, F27, F28, F36, F49                                 | `&&` / `||` short-circuit gating a loop's break condition; lambda side-effect proves no eager eval.   |
| `everything.bu`               | F1–F4, F7–F12, F14–F22, F24–F49                                            | A single program that hits every feature in some form. Catch-all for the matrix.                      |

### Error-Path Programs (compile-time failures)

These are loaded the same way but expected to fail at a specific stage,
with a header line `EXPECT_FAIL: <stage>` (e.g. `parse`, `typecheck`).
Add as needed; not all invariants need a dedicated integration program
(many are already covered by unit tests).

| Program                       | Invariant Tested | Stage Expected to Fail |
|-------------------------------|------------------|------------------------|
| `bad_no_main.bu`              | I1               | typecheck              |
| `bad_main_returns_unit.bu`    | I1               | typecheck              |
| `bad_break_outside_loop.bu`   | I2               | typecheck              |
| `bad_assign_immutable.bu`     | I3               | typecheck              |
| `bad_while_int_condition.bu`  | I4               | typecheck              |

---

## Coverage Audit

After the matrix is implemented, each feature in the F-list should appear
at least once in the "Features Covered" column above. The audit is
mechanical: grep for `F<n>,` and confirm at least one match. Features
with no integration coverage are flagged for follow-up — the goal is
not 100% feature-per-program but at least one program per feature.

Features that are tricky to integrate cleanly (e.g., F6 nested block
comments) may be left to unit-test coverage only; mark such features
explicitly in this section.

**Out-of-scope for integration tests** (unit-test-only):
- F5, F6 — comments are stripped by the lexer; their behavior is
  verified by `lexer_test`. Integration programs may use comments but
  don't *test* them.

---

## Adding a New Feature

When a new language feature lands:

1. Add an entry at the end of the F-list above (stable numbering).
2. Add at least one integration program that exercises it in
   combination with one or two existing features.
3. Update `everything.bu` to include the feature where it makes sense.
4. Add unit tests covering the feature's edge cases as usual.

The integration matrix is for *combinatorial* coverage; unit tests
remain the place for boundary conditions and error wording.
