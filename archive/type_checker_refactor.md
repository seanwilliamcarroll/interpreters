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
| `hir/type_folder.hpp:50-51` | Generic `fold`: NeverType returns `m_never`; `walk` has no branch (no-op) |
| `zir/context.hpp:62-63` | Pass-through HIR → ZIR |

## Approach

A coherent Never story has four pieces:

**Semantics**: Never is the type an expression has when it produces no
value for its immediate context. That's separate from whether the
expression *transports* a value via control flow to a *carrier slot*
elsewhere. `panic!()` is Never with no carrier. `return e` is Never; `e`
flows to the function-return carrier. `break e` is Never; `e` flows to
the enclosing-loop's result carrier. `loop {}` with no reachable break
is Never. The carrier-slot framing is what lets return / break / panic /
loop {} share one type-level story while differing only in what they
ferry away.

**Type-level**: bust uses two operations on types — `unify` (HM
equation-solving; Never short-circuits as a no-op) and `join` (least
upper bound, needed for if/match/loop result types; Never is absorbed).
Each operation hand-codes its Never rule today (three sites in
`hir/type_unifier.hpp`); that's adequate for the current algebra and
not worth a bottom-type encoding. Carrier slots are themselves joined
across all contributors (every `return e` in a function, every
`break e` in a loop).

**Inference boundary**: when inference finishes, residual unconstrained
type variables that flowed from a Never expression default to Unit.
Single rule, one place. Equivalent to Rust's never-type fallback.

**Codegen**: a Never value is zero-sized and unreachable at runtime.
Codegen treats it as no-op-on-store / unreachable-on-load. The mangler
only needs a canonical name for Never if Never appears in a mangled
signature — not necessary until `!` is user-facing as a return type.

## What landed

- **TypeFolder consolidation.** Three near-identical visitors (`TypeVariableSubstituter`, `TypeVariableCollapser`, `FreeTypeVariableCollector`) collapsed into one function+functor pair (`fold` / `walk`) at `hir/type_folder.hpp`. Callers reduced to small lambdas at `hir/type_variable_substituter.hpp`, `hir/type_variable_collapser.hpp`, and `hir/free_type_variable_collector.hpp`.
- **Semantics framing.** The carrier-slot model (above) ties return / break / panic / loop {} into one type-level story.
- **Implicit invariant check.** The `InternalCompilerError` throws at `mono/name_mangler.hpp:75-77` and `zir/context.hpp:62-63` document the "no `TypeVariable` past TypeChecker" invariant. Tests exercise generalization paths heavily without hitting them.
- **Reachability cleanup.** Per-arm sanity asserts in `IfExpr` codegen were dropped; codegen now relies on `current_block_terminated`.

## Deferred — gated on concrete motivation

- **Generalize the never-type fallback.** The local form at `hir/statement_checker.cpp:56-61` covers every case bust hits today. Promote to an end-of-TypeChecker pass when a pattern leaks past it (likely needs generic containers or user-visible `!` first). Implementation would build on `walk` / `fold`.
- **Reachability as `Block::is_divergent` field.** Codegen would read precomputed instead of tracking `current_block_terminated`. Open only if codegen state-tracking starts hurting.

## Direction — trait/class system, not subtyping

Subtyping is not bust's path. Its only payoff here is the Never special-case cleanup — three small sites in `hir/type_unifier.hpp`. The cost is converting inference from equation-solving to inequation-solving, a fundamental algorithmic shift Rust spent years debugging.

The next type-system addition is a **trait/class system**, integrated *additively* with HM: trait obligations become a separate constraint kind resolved by a separate solver, leaving unification untouched. Bust already has the seed (`PrimitiveTypeClass`).

Motivating cases — none exist yet, but on the natural roadmap:

- Generic numeric code: `fn sum<T: Add>(xs: Vec<T>) -> T`
- User-defined `Eq` / `Ord` / `Display`
- Iteration protocol

When trait machinery grows enough to feel cramped sharing space with the unifier, that's the signal to split `TypeUnifier` and `TypeClassResolver` into separate composable pieces.
