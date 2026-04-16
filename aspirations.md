# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Type System

- [x] Collapse unified type variables to their root before generalization
  — When `x + y` unifies `?T<0>` and `?T<1>`, the unifier knows they're
  the same type but stores them as two entries pointing to the same root.
  Before generalization decides which variables are polymorphic, substitute
  all unified variables with their root representative so the system sees
  one variable, not two aliases. Without this, monomorphization would try
  to substitute two independent type parameters when there's really one.
  Prerequisite for monomorphization (see Pipeline Overhaul).

## New Types and FFI

- [x] char, i8, i32, and cast expressions (landed)
- [x] Extern function declarations (syntax, parsing, type checking, codegen)
- [x] `putchar` via libc

  Extern declarations → putchar

## Aggregate Types

- [ ] Arrays (fixed-size)
- [ ] Tuples
- [ ] Strings (likely built on arrays or a dedicated type)

## User-Defined Types

- [ ] Structs (declaration, field access, construction)
- [ ] Enums / tagged unions

## Codegen (LLVM IR)

- [ ] Lambda expressions in LLVM IR generation
- [x] Non-i64 integer types in codegen
- [x] Cast expressions in codegen
- [ ] Optimizations (LLVM pass pipeline, inlining, etc.)

## Control Flow / Return Analysis

- [ ] Smarter reasoning about `return` expressions in blocks
  — Currently, `return expr;` with a trailing semicolon makes it a statement,
  so the block's final expression is absent and the block type is unit. This
  means `fn foo() -> i64 { return 42; }` is a type error because the block
  returns `()`, not `i64`. Rust handles this by special-casing return/diverging
  statements. We should either: (a) treat `return` as a diverging expression
  (type `!`) so the block type is still valid, or (b) allow semicolons after
  the final expression in a block without forcing the block type to unit.
  Either approach would make `return x + y;` legal in tail position.

## Type Unifier

- [ ] Make `TypeUnifier::find` const via `mutable` on the union-find internals
  — Path compression mutates the parent pointers during `find`, which is why
  it currently can't be `const`. This is the textbook use case for `mutable`:
  the logical result is unchanged, only an internal cache is rewritten.
  Marking the union-find storage `mutable` lets const callers (e.g. a
  post-type-check resolution pass) invoke `find` while preserving the
  path-compression optimization.

## Pipeline Overhaul

Goal: polymorphic lambdas work end-to-end through codegen.

- [x] Drop evaluator (remove from CMake build, evaluator no longer maintained)
- [x] Monomorphization pass
  - Sits between type checking and zonking
  - Canonicalization → monomorphization → polymorphic lambdas resolve cleanly
- [ ] ZIR: arena-based zonked intermediate representation
  - New representation produced by the zonker, replacing reuse of `hir::Program`
  - Expression arena (flat `ExprId` indexing instead of `unique_ptr<Expr>`)
  - Concrete types only, no `UnifierState`
  - Codegen reads ZIR instead of HIR
- [ ] Codegen targets ZIR
  - Migrate codegen to consume ZIR instead of `hir::Program`

  Drop evaluator → monomorphization → ZIR → codegen targets ZIR

  The HIR (with `unique_ptr` trees) stays as-is for type checking and
  monomorphization. The zonker becomes a lowering pass from HIR to the
  arena-backed ZIR. Read-only passes after zonking (codegen, future
  optimizations) work against the arena.
