# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Type System

- [ ] Collapse unified type variables to their root before generalization
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

## Pipeline Overhaul

Goal: polymorphic lambdas work end-to-end through codegen.

- [ ] Drop evaluator (remove from CMake build, evaluator no longer maintained)
- [ ] Monomorphization pass — see [monomorphization.md](monomorphization.md)
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
