# Feature Roadmap — Brainstorm & Dependency Map

Open brainstorming around the next batch of language features. Goal is to
sketch what depends on what, rough effort, and where the natural sequencing
lies — not to commit to an order yet.

## Questions in scope

- Tuples in codegen
- User-defined types (structs)
- Mutability + assignment operator
- Loops (revisit while / for) — beneficiary of mutability
- References (`&T`, `&mut T`) and the borrow checker
- Pattern matching
- Algebraic data types (enums / sum types)
- Traits Stage 1 and Stage 2 (from earlier discussion)
- Global let bindings
- Closure refactor (already in `closures_plan.md`)

## Dependency graph

```
[tuples-codegen]──┐
                  ├─→ [pattern-matching v0] ─┐
                  │                          │
[structs] ────────┼─→ [methods] ─→ [traits-S1] ─→ [traits-S2 / constrained generics]
                  │                          │
                  └─→ [ADTs] ←───────────────┘
                       │
[mutability + =] ──┬─→ [loops cleanup]
                   ├─→ [references no-check] ─→ [borrow checker]
                   └─→ [mutable globals]

[const eval] ─→ [global lets]

[closure refactor]   (orthogonal; merges into traits-S2 as `Fn`)
```

## Per-feature notes

### Tuples in codegen
- **Status:** grammar, parser, AST, HIR, zir, type checker, monomorphizer all landed in PR #44. Codegen `TupleExpr` / `DotExpr` operators are still stubs at `expression_generator.cpp:82`.
- **Blocks on:** nothing — just finish codegen
- **Enables:** pattern matching test bed; ranges as `(start, end)`; multi-return ergonomics
- **Size:** S

### Pattern matching
- **Blocks on:** at least tuples for the first interesting case
- **Enables:** ADTs (which are useless without it), ergonomic destructuring in `let`
- **Size:** M for the engine; staged extensions
- **Strategy:** build once, extend by pattern kind: `_` → literal → identifier → tuple → struct → enum-variant. Decision-tree compilation. Exhaustiveness checking is a separate subproject (Maranget's algorithm or simpler).

### Mutability + assignment
- **Blocks on:** nothing
- **Enables:** real `while`/`for`, mutable globals, `&mut`, all method work that takes `&mut self`
- **Size:** M
- **Notes:** HIR adds a mut flag on bindings. Type checker enforces. Codegen barely changes — alloca-everything is already in place; just emit a store on assignment. Place-expression / value-expression distinction starts mattering here (precursor to references).

### Loops cleanup
- **Status:** `while` is an empty AST stub (`bust/ast/nodes.hpp:106`); nothing flows through HIR/zir/codegen. `for` doesn't exist at all.
- **Blocks on:** mutability for usefulness; for `for x in xs`, traits Stage 2 + an `Iterator` trait.
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
- **Notes:** see earlier discussion. Monomorphization (already on `aspirations.md`) is the codegen path. Closure refactor's `CallableType` collapses into "`Fn` is a built-in trait" once this lands.

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

1. **Tuples in codegen** [S] — quick win, unblocks PM
2. **Mutability + assignment** [M] — foundational, unblocks loops + references
3. **Loops cleanup** [S] — beneficiary of (2)
4. **Pattern matching v0** (literals + tuples + bindings) [M] — engine ready before structs/ADTs need it
5. **Structs** [M] — user-defined types
6. **Methods (inherent impls)** [M] — natural step before traits
7. **References without checker** [M] — needed for idiomatic `&self`
8. **ADTs** [L] — extends PM, gives `Option`/`Result`
9. **Traits Stage 1** [M] — replaces `PrimitiveTypeClass`
10. **Traits Stage 2 / constrained generics** [L] — real polymorphism
11. **Closure refactor** — slot in anywhere; especially clean after S2
12. **Global lets (`static`)** — slot in anywhere
13. **Borrow checker** [XL] — capstone

Alternative: do **structs + methods + traits-S1** earlier (slot 2–4) if you want
the trait machinery in mind sooner; cost is delaying the loops/PM payoff.

## Cross-cutting questions

- **Generics syntax timing.** `Foo<T>` and `fn foo<T>(...)` could land with structs (Stage 1: unconstrained generics) or wait for traits-S2 (constrained). Easier to add generics when constraints arrive — saves a re-think.
- **Const-eval scope.** Comes up for `const`, array lengths, generic const params. Probably defer all of it.
- **Place-vs-value lvalue model.** Every feature past mutability assumes it. Worth nailing the HIR representation when mutability lands rather than retrofitting.
- **Visibility / modules.** Not on this list. At some point `pub`/`mod` start mattering for trait coherence — flag for later.

## Final priority list

Ordered, with rough sequencing rationale. Each item should be substantially
complete before moving on; small overlaps and opportunistic out-of-order work
are fine.

1. **Finish tuples in codegen** — already landed everywhere except codegen;
   tiny finish, highest leverage-to-cost. Becomes the test bed for pattern
   matching and is needed for any aggregate-shaped feature later.
2. **Mutability + assignment operator** — foundational. Cheap because of
   alloca-everything. Unblocks `while`, `&mut`, and forces the place-vs-value
   model the rest of the language needs.
3. **`while` loops** — direct beneficiary of (2). `for` is *deferred* to
   after traits-S2 so it lands as iterator-trait sugar from day one — no
   C-style stopgap to retrofit.
4. **Pattern matching v0** (literals, identifiers, tuples, irrefutable in
   `let`) — build the engine while requirements are simple; extend per
   pattern kind as new shapes land.
5. **Structs** — first user-defined type. Codegen already has `StructType`.
6. **Methods (inherent impls)** — `Self`, method resolution. Needs (7) for
   `&self`, but the dispatch machinery can stage before references.
7. **References without borrow checker** — `&T` / `&mut T` with C-pointer
   semantics; place-vs-value model; auto-ref/auto-deref. Unblocks idiomatic
   `&self` for (6).
8. **ADTs (enums)** — extends pattern matching; gives `Option` / `Result`.
   Tag-only enums first, payloads next.
9. **Traits Stage 1** (monomorphic only) — replaces `PrimitiveTypeClass`
   with real user-declarable traits.
10. **Traits Stage 2** (constrained generics + qualified types) — real
    polymorphism. Re-enables `make_adder` cleanly via `Fn` as a trait, and
    retires the closure bandaid.
11. **`for` loops via `Iterator` trait** — direct payoff from (10).
    Desugars to `IntoIterator::into_iter` + `Iterator::next` calls.
12. **Borrow checker** — capstone; multi-month subproject of its own.

Slot in opportunistically:

- **Global lets (`static`)** — anytime after (2); useful but not blocking
  anything else.
- **Generic structs / functions (unconstrained)** — could land with (5),
  but cleaner to wait until (10) so constraints are in scope from the start.
- **Closure narrow fix** — only if (10) gets pushed out; otherwise let
  traits-S2 absorb it.
