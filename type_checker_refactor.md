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
| `hir/type_unifier.hpp:372-383` | `join(τ_a, τ_b)`: Never on either side → take other; otherwise unify+find |
| `hir/block_checker.cpp:75-80` | Block-Never rule: any diverging statement → block is Never |
| `hir/statement_checker.cpp:56-61` | Local never-fallback: unannotated `let` whose body is Never → annotate as Unit |
| `mono/name_mangler.hpp:75-77` | Throws on Never (and TypeVariable) |
| `codegen/expression_generator.cpp:156-175` | Block: bail with empty Value if a statement or final expression terminated the block |
| `codegen/expression_generator.cpp:177-301` | If-expr: per-arm divergence flags drive whether to emit store/jump/merge |
| `codegen/expression_generator.cpp:396-415` | `is_signed_type` lumps Never with non-numeric, throws |
| `codegen/statement_generator.cpp:38-41,78-81` | Let/Assignment: skip alloca/store if RHS terminated |
| `codegen/expression_generator.cpp:689-695,732-738` | Lambda body: skip implicit return if body terminated |
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

- [ ] Decide where unifier-substitution (collapse) is applied: either consolidate scattered per-binding `TypeVariableCollapser` calls into one explicit pre-mono pass, or fold `find()` into the monomorpher's walk. Either way, catch surviving tvars at one well-defined boundary
- [ ] Convert "no TypeVariable" runtime errors in `mono/name_mangler.hpp` and `zir/context.hpp` to asserts once that boundary exists

### Tier 2 — medium, good payoff

- [ ] Unify `TypeVariableSubstituter` / `TypeVariableCollapser` / `FreeTypeVariableCollector` under a single `TypeFolder<F>` abstraction
- [ ] Generalize the never-type fallback. A local version landed in `hir/statement_checker.cpp:56-61` for let-bindings; promote to a single end-of-TypeChecker pass over residual tvars that flowed from Never
- [ ] Move reachability into HIR as explicit field on `Block` (`is_divergent: bool`); codegen reads instead of recomputing via `current_block_terminated`. Would also let codegen drop the per-arm sanity-check asserts in `IfExpr`
- [ ] Decide canonical mangle name for Never (`bot`); update mangler to accept it

### Tier 3 — larger, deferred

- [ ] Split `TypeUnifier` and `TypeClassResolver` (constraint side) into separate composable pieces
- [ ] Introduce subtyping/coercion as a first-class concept; add `is_subtype(τ_from, τ_to)`; let Never participate as real bottom; usable by literal coercion (i64-literal to i32 in context), auto-deref, etc.
