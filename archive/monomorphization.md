# Monomorphization

## Problem

The type checker supports let-polymorphism: unconstrained type variables in
let-bound lambdas are generalized and instantiated with fresh variables at each
use site. This means polymorphic definitions (e.g., the identity function) carry
type variables that are never resolved to concrete types.

The zonker assumes all type variables resolve to concrete types. Polymorphic
definitions violate this, causing an ICE in the zonk pass.

```rust
fn main() -> i64 {
  let g = |h| { h };       // g: forall T. T -> T
  let f = |x, y| { x + y };
  g(f)(4, 8)               // g instantiated at (i64, i64) -> i64
}
```

`g`'s definition-site type variable is never resolved — only its fresh
instantiations are.

## Approach

For each polymorphic definition, stamp out a specialized copy for every concrete
type it is instantiated at. After monomorphization, no type variables remain.

## Prerequisites

- Collapse unified type variables to their root before generalization.
  When `x + y` unifies `?0` and `?1`, substitute both with their shared
  root so generalization sees one polymorphic variable, not two aliases
  that it would try to substitute independently during monomorphization.
- Propagate type class constraints through `create_fresh_type_vars` (done).
- Filter resolved type variables from generalization (done).

## Work Items

### 1. Instantiation tracking

During type checking, record each instantiation of a polymorphic type scheme.
When `create_fresh_type_vars` fires, store the mapping from the original free
variables to the concrete types they eventually resolve to. This needs to be
collected after unification completes for the enclosing scope — the fresh
variables may not be resolved at the point of instantiation.

### 2. HIR cloning with type substitution

Deep-copy a LambdaExpr (and its full expression tree), replacing type variables
with concrete types. Requires clone methods across all HIR node types since they
use `unique_ptr`. Each cloned node's TypeId must point to a valid entry in the
type registry.

### 3. Call site rewriting

Replace references to polymorphic definitions with references to their
specialized copies. A polymorphic function used at three different types
produces three specialized definitions and three distinct call site rewrites.

### 4. Monomorphization pass

New pass between type checking and zonking. Walks all top-level items and
let bindings, finds polymorphic definitions, and for each recorded
instantiation:
- Clones the definition with concrete type substitutions
- Rewrites call sites to reference the specialized copy
- Produces a program with no remaining type variables

The zonk pass then becomes a validation-only step (or is removed).

### 5. Name mangling

Each specialized copy needs a unique name for codegen. Something like
`g_fn_i64_i64_to_i64` or a simpler numeric suffix scheme.

## Edge Cases

- **Nested polymorphism**: `g(f)` where both are polymorphic — `g` is
  specialized first, which may reveal the concrete type for `f`.
- **Multiple instantiations**: Same function used at different types in
  different let bindings or call sites.
- **Unused polymorphic definitions**: No instantiation recorded — can be
  dropped or flagged as dead code.
- **Recursive self-calls at the same type**: Safe (single specialization).
  Recursive calls at different types would not terminate — not currently
  possible in bust but worth guarding against.
