# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Type System

- [ ] Canonicalize unified type variables before generalization (zonk at the
      generalization point so unified-but-unresolved variables share a root)
- [ ] Monomorphization — see [monomorphization.md](monomorphization.md)
  - Instantiation tracking
  - HIR cloning with type substitution
  - Call site rewriting
  - Monomorphization pass (between type checking and zonking)
  - Name mangling

  Canonicalization → monomorphization → polymorphic lambdas work end-to-end
  through codegen.

## New Types and FFI

- [x] char, i8, i32, and cast expressions (landed)
- [ ] Extern function declarations (syntax, parsing, type checking)
- [ ] `putchar` via libc

  Extern declarations → putchar

## Codegen (LLVM IR)

- [ ] Lambda expressions in LLVM IR generation
- [ ] Non-i64 integer types in codegen
- [ ] Cast expressions in codegen
- [ ] Optimizations (LLVM pass pipeline, inlining, etc.)

## HIR Representation

- [ ] Expression arena (flat `ExprId` indexing instead of `unique_ptr<Expr>`)
  — large refactor, makes cloning for monomorphization trivial but not a
  prerequisite for it
