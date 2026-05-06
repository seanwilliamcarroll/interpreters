# Feature Roadmap — Brainstorm & Dependency Map

Open brainstorming around the next batch of language features. Goal is to
sketch what depends on what, rough effort, and where the natural sequencing
lies — not to commit to an order yet.

## Questions in scope

- User-defined types (structs)
- Loops (`while` / `for`)
- References (`&T`, `&mut T`) and the borrow checker
- Pattern matching
- Algebraic data types (enums / sum types)
- Traits Stage 1 and Stage 2 (from earlier discussion)
- Global let bindings
- Closure refactor (already in `closures_plan.md`)

## Dependency graph

(Tuples and mutability already landed; arrows from them are kept to show
what they unblock.)

```
[tuples]──┐
          ├─→ [pattern-matching v0] ─┐
          │                          │
[structs] ┼─→ [methods] ─→ [traits-S1] ─→ [traits-S2 / constrained generics]
          │                          │
          └─→ [ADTs] ←───────────────┘
                │

[mutability] ──┬─→ [while loops]
               ├─→ [references no-check] ─→ [borrow checker]
               └─→ [mutable globals]

[const eval] ─→ [global lets]

[closure refactor]   (orthogonal; merges into traits-S2 as `Fn`)
```

## Per-feature notes

### Tuples in codegen
- **Status:** done. Construction, projection, function arguments, and tuple-returning functions all work end-to-end; covered by `bust.codegen.tuples`.
- **Enables:** pattern matching test bed; ranges as `(start, end)`; multi-return ergonomics
- **Size:** S

### Pattern matching
- **Blocks on:** at least tuples for the first interesting case
- **Enables:** ADTs (which are useless without it), ergonomic destructuring in `let`
- **Size:** M for the engine; staged extensions
- **Strategy:** build once, extend by pattern kind: `_` → literal → identifier → tuple → struct → enum-variant. Decision-tree compilation. Exhaustiveness checking is a separate subproject (Maranget's algorithm or simpler).

### Mutability + assignment
- **Status:** done. `let mut`, immutable-binding rejection, lambda-parameter mutability, captured-binding mutability checks all land in HIR; codegen emits stores on `Assignment`. Covered by `bust.codegen.expressions` and `bust.type_checker` (search for `let mut`).
- **Enabled:** real `while`/`for`, mutable globals, `&mut`, all method work that takes `&mut self`

### Loops cleanup
- **Status:** `while` and `for` are empty AST stubs (`bust/ast/nodes.hpp:112-113`); nothing flows through HIR/zir/codegen.
- **Blocks on:** nothing for `while` (mutability already landed); `for x in xs` waits on traits Stage 2 + an `Iterator` trait.
- **Size:** S for `while`. `for` waits for traits-S2 — no C-style stopgap.
- **Notes:** doing `for` right means desugaring through an iterator trait. Skipping the C-style detour avoids retrofit work.

### Structs (user-defined product types)
- **Blocks on:** nothing — codegen has `StructType` already (used for closures)
- **Enables:** methods, trait impls, ADT payloads
- **Size:** M
- **Notes:** parser + HIR work is the bulk. Decisions: positional-init vs named-init, field privacy (skip for now), generic structs (`Foo<T>`) probably want to wait until traits-S2 is on the table since it's the same machinery.

### Methods (inherent impls, no traits yet)
- **Blocks on:** structs
- **Enables:** idiomatic method-call syntax; bridge to traits
- **Size:** M
- **Notes:** introduces `Self` type and method resolution. `&self` / `&mut self` need references. Possibly start with `fn foo(self: Foo)` (no auto-ref) to stage the work.

### References (`&T`, `&mut T`) — no borrow checker
- **Blocks on:** mutability (for `&mut`)
- **Enables:** `&self` methods, ergonomic loops, eventually borrow checker
- **Size:** M
- **Notes:** place-vs-value distinction lands here for real. Deref operator, auto-ref/auto-deref at method call sites. No safety yet — semantics are C-pointer-ish. Pattern `&x` for ref-patterns.

### Borrow checker
- **Blocks on:** references
- **Enables:** the actual Rust safety story
- **Size:** XL — multi-month
- **Notes:** lifetimes (inference + parameters), NLL-style dataflow, aliasing rules. Genuinely its own subproject. Worth deferring until everything else is solid.

### Algebraic data types (enums)
- **Blocks on:** pattern matching, structs (for variant payloads)
- **Enables:** `Option<T>`, `Result<T, E>`, real expressivity
- **Size:** L
- **Notes:** representation is `{ tag, union-of-payloads }`. Sub-stages: (1) tag-only C-like enums [S], (2) variants with payloads [M], (3) generic enums [needs traits-S2 generics]. PM and ADTs co-evolve — extend the pattern engine as variant kinds land.

### Traits Stage 1 (monomorphic only)
- **Blocks on:** structs (impl targets); ideally methods (traits generalize them)
- **Enables:** replace `PrimitiveTypeClass`; user-defined operator-style overloading
- **Size:** M
- **Notes:** impl table keyed on `(trait, type)`. No generics yet — every call site has a concrete receiver.

### Traits Stage 2 (constrained generics)
- **Blocks on:** traits-S1, HM extension to qualified types
- **Enables:** real polymorphism, `Iterator`, idiomatic `for`, `Fn` as a trait
- **Size:** L (3–4 weeks of focused work)
- **Notes:** see earlier discussion. Monomorphization is already implemented in `bust/mono/` and is the codegen path. Closure refactor's `CallableType` collapses into "`Fn` is a built-in trait" once this lands.

### Global let bindings
- **Blocks on:** for `const`, a const-evaluator; for `static`, an init strategy
- **Enables:** top-level constants, mutable globals
- **Size:** S–M
- **Notes:** Rust splits `const` (compile-time eval, copied at use) and `static` (one storage slot, runtime init). Easier path: `static`-only with init via `@llvm.global_ctors`. `const` needs a const-eval pass — defer.

### Closure refactor
- **Status:** the bandaid version is merged (PRs #41, #42). `make_adder`-style higher-order closures still fail and the test in `bust/test/src/codegen_lambdas_test.cpp` is commented out. Resolution is type-system tracking of FnPtr-vs-Closure (see `closures_plan.md`).
- **Blocks on:** traits Stage 2 cleans this up most naturally (`Fn` becomes a built-in trait); a narrower fix is also possible without traits.
- **Enables:** re-enabling the disabled test; closures returned from functions; closures passed through generic parameters.
- **Size:** M (narrow fix) or folded into traits-S2 (broader).

## Suggested ordering (one possible path)

A "Rust-shaped" path that minimizes blocked work:

1. **`while` loops** [S] — direct beneficiary of mutability, which already landed
2. **Pattern matching v0** (literals + tuples + bindings) [M] — engine ready before structs/ADTs need it
3. **Structs** [M] — user-defined types
4. **Methods (inherent impls)** [M] — natural step before traits
5. **References without checker** [M] — needed for idiomatic `&self`
6. **ADTs** [L] — extends PM, gives `Option`/`Result`
7. **Traits Stage 1** [M] — replaces `PrimitiveTypeClass`
8. **Traits Stage 2 / constrained generics** [L] — real polymorphism
9. **Closure refactor** — slot in anywhere; especially clean after S2
10. **Global lets (`static`)** — slot in anywhere
11. **Borrow checker** [XL] — capstone

Alternative: do **structs + methods + traits-S1** earlier (slot 1–3) if you want
the trait machinery in mind sooner; cost is delaying the loops/PM payoff.

## Cross-cutting questions

- **Generics syntax timing.** `Foo<T>` and `fn foo<T>(...)` could land with structs (Stage 1: unconstrained generics) or wait for traits-S2 (constrained). Easier to add generics when constraints arrive — saves a re-think.
- **Const-eval scope.** Comes up for `const`, array lengths, generic const params. Probably defer all of it.
- **Place-vs-value lvalue model.** Single-variant `Place` exists in HIR and ZIR (`hir/nodes.hpp:128`, `zir/nodes.hpp:69`) — only `Identifier` for now. References, field assignment, and indexed assignment will each broaden the variant.
- **Visibility / modules.** Not on this list. At some point `pub`/`mod` start mattering for trait coherence — flag for later.

## Final priority list

Ordered, with rough sequencing rationale. Each item should be substantially
complete before moving on; small overlaps and opportunistic out-of-order work
are fine.

1. **`while` loops** — direct beneficiary of mutability. `for` is *deferred*
   to after traits-S2 so it lands as iterator-trait sugar from day one — no
   C-style stopgap to retrofit.
2. **Pattern matching v0** (literals, identifiers, tuples, irrefutable in
   `let`) — build the engine while requirements are simple; extend per
   pattern kind as new shapes land.
3. **Structs** — first user-defined type. Codegen already has `StructType`.
4. **Methods (inherent impls)** — `Self`, method resolution. Needs (5) for
   `&self`, but the dispatch machinery can stage before references.
5. **References without borrow checker** — `&T` / `&mut T` with C-pointer
   semantics; place-vs-value model; auto-ref/auto-deref. Unblocks idiomatic
   `&self` for (4).
6. **ADTs (enums)** — extends pattern matching; gives `Option` / `Result`.
   Tag-only enums first, payloads next.
7. **Traits Stage 1** (monomorphic only) — replaces `PrimitiveTypeClass`
   with real user-declarable traits.
8. **Traits Stage 2** (constrained generics + qualified types) — real
   polymorphism. Re-enables `make_adder` cleanly via `Fn` as a trait, and
   retires the closure bandaid.
9. **`for` loops via `Iterator` trait** — direct payoff from (8).
    Desugars to `IntoIterator::into_iter` + `Iterator::next` calls.
10. **Borrow checker** — capstone; multi-month subproject of its own.

Slot in opportunistically:

- **Global lets (`static`)** — anytime; useful but not blocking anything else.
- **Generic structs / functions (unconstrained)** — could land with (3),
  but cleaner to wait until (8) so constraints are in scope from the start.
- **Closure narrow fix** — only if (8) gets pushed out; otherwise let
  traits-S2 absorb it.
