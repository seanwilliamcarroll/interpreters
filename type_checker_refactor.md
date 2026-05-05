# Type Checker Pipeline Refactor

## Problem

The type-checking pipeline (HIR / mono / ZIR) handles `Never` ad hoc.
Every downstream pass invents its own Never-handling story, leading to
duplication, divergent invariants, and ICEs at the boundaries (e.g. the
mango mangler chokes on `let x = if true { return 1; } else { return 3; };`).

Beyond Never, the pipeline has accreted three near-identical type
visitors, an incomplete block-divergence rule, an open-coded `join`
inside the if-checker, and reachability information that's recomputed at
each layer. Worth a structured pass before more diverging constructs
(`break`, `panic!`, `loop {}`) land.

## Where Never lives today

| Site | Behavior |
|------|----------|
| `hir/type_unifier.hpp:71-74` | `unify(τ, Never)` short-circuits |
| `hir/type_unifier.hpp:141-143` | `unify(α, Never)` short-circuits |
| `hir/expression_checker.cpp:408-410` | Hand-rolled `join(then, else)` via ternary |
| `hir/block_checker.cpp:73-80` | Block-Never rule fires only when no final-expression |
| `mono/name_mangler.hpp:76` | Throws on Never |
| `codegen/expression_generator.cpp:173` | Re-derives "if then is Never, use else's type" |
| `codegen/expression_generator.cpp:343` | `is_signed_type` lumps Never with non-numeric, throws |
| `hir/type_variable_substituter.hpp:80-82` | Pass-through |
| `hir/type_variable_collapser.hpp:60-62` | Pass-through |
| `hir/free_type_variable_collector.hpp:50` | Pass-through |
| `zir/context.hpp:62-63` | Pass-through HIR → ZIR |

## Approach

A coherent Never story has three pieces:

**Type-level**: Never is the bottom of a tiny subtype lattice. The
operations the system uses are `unify` (HM equation-solving — Never as
no-op is correct), `join` (least upper bound — needed for if/match/loop
result types), and eventually `is_subtype` (Never coerces to anything).

**Inference boundary**: when inference finishes, residual unconstrained
type variables that flowed from a Never expression default to Unit.
Single rule, one place. Equivalent to Rust's never-type fallback.

**Codegen**: a Never value is zero-sized and unreachable at runtime. The
mangler gives it a canonical name; codegen treats it as
no-op-on-store / unreachable-on-load.

## Tasks

### Tier 1 — small, immediate, high-leverage

- [ ] Fix block-divergence rule in `hir/block_checker.cpp:73-80`: always Never if any statement diverged, regardless of final expression
- [ ] Add `TypeUnifier::join(τ_a, τ_b)` with rules: Never on either side → take other; otherwise unify and return one
- [ ] Replace ternary at `hir/expression_checker.cpp:408-410` with `join` call
- [ ] Add explicit collapse-to-concrete pass between TypeChecker and Monomorpher: walk program, replace every TypeVariable with `unifier.find(α)`, throw if any tvar survives
- [ ] Convert "no TypeVariable" runtime errors in `mono/name_mangler.hpp` and `zir/context.hpp` to asserts after collapse pass lands

### Tier 2 — medium, good payoff

- [ ] Unify `TypeVariableSubstituter` / `TypeVariableCollapser` / `FreeTypeVariableCollector` under a single `TypeFolder<F>` abstraction
- [ ] Introduce never-type fallback at end of TypeChecker: residual tvars that flowed from Never default to Unit
- [ ] Move reachability into HIR as explicit field on `Block` (`is_divergent: bool`); codegen reads instead of recomputing via `current_block_terminated`
- [ ] Decide canonical mangle name for Never (`bot`); update mangler to accept it
- [ ] Decide codegen rule for Never values: skip emission at let-binding boundary when RHS type is Never

### Tier 3 — larger, deferred

- [ ] Decide ZIR's relationship to HIR types: commit to per-primitive variants and document why, or merge back to a single `TypeKind` with a "no TypeVariable" invariant
- [ ] Split `TypeUnifier` and `TypeClassResolver` (constraint side) into separate composable pieces
- [ ] Introduce subtyping/coercion as a first-class concept; add `is_subtype(τ_from, τ_to)`; let Never participate as real bottom; usable by literal coercion (i64-literal to i32 in context), auto-deref, etc.

### Test coverage to add alongside

- [ ] `let x = if true { return 1; } else { return 3; };` (both arms diverge)
- [ ] `let x = return 5; <unreachable use of x>` (RHS-diverges before binding)
- [ ] `if cond { return 1 } else { return 3 }` in trailing position (block diverges)
- [ ] Nested diverging let inside lambda body
- [ ] Diverging arm in higher-order function call argument
