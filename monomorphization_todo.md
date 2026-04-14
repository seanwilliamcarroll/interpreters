# Monomorphization TODO

Pipeline after this work:

```
Parser → AST
  → TypeChecker → HIR (with instantiation records)
  → Monomorphizer → HIR (all definitions monomorphic, no free type vars)
  → Zonker → ZIR (future: arena-based, concrete types only)
  → CodeGen → LLVM IR
```

Monomorphization reuses `hir::Program`, `hir::nodes`, and `hir::TypeRegistry`
entirely. No new node types. The output is still an `hir::Program` — just one
where every definition is specialized to concrete types. The zonker then cleans
up any remaining type variables and (eventually) lowers to ZIR.

---

## Phase 1: Instantiation Tracking (in type checker)

The type checker already calls `create_fresh_type_vars` when a polymorphic
`TypeScheme` is used at a call site. We need to record what happened.

- [ ] Define an `InstantiationRecord` struct
  - Source: the let-bound name being instantiated
  - Mapping: original free type variables → fresh type variables created
  - The fresh vars aren't resolved yet at this point — they resolve later
    during unification of the call expression

- [ ] Record instantiations in `Context::create_fresh_type_vars`
  - Store each `InstantiationRecord` in a side table on `Context`
  - Only fires for `TypeScheme`s with non-empty `m_free_type_variables`

- [ ] Resolve instantiation records after type checking completes
  - Walk the recorded mappings and chase each fresh type variable through
    the unifier to its concrete type
  - Result: for each polymorphic name, a list of concrete type substitutions
    e.g. `"id" → [{?T<0> → i64}, {?T<0> → bool}]`

- [ ] Carry resolved instantiation data forward in `hir::Program`
  - Add a field to `Program` (or `UnifierState`) so the monomorphization
    pass can read it
  - Could be: `std::unordered_map<std::string, std::vector<TypeSubstitution>>`
    where `TypeSubstitution = std::unordered_map<TypeVariable, TypeId>`

- [ ] Tests: verify instantiation records are correct
  - Single use: `let f = |x| { x }; f(1)` → one record, `?T → i64`
  - Multiple uses: `let f = |x| { x }; f(1); f(true)` → two records
  - No free vars: `let f = |x: i64| { x }; f(1)` → no records
  - Nested: `let g = |f| { f }; let h = |x| { x }; g(h)(1)` → records for
    both `g` and `h`

---

## Phase 2: Monomorphization Pass

New pass in `bust/mono/`. Reads `hir::Program` with instantiation data,
produces `hir::Program` with all polymorphism resolved.

### Name Mangling

- [ ] Design a mangling scheme
  - Needs to produce unique names per specialization
  - Must be valid identifiers for codegen (LLVM global names)
  - Simple approach: `{original_name}_mono_{type1}_{type2}_{...}`
  - e.g. `id_mono_i64`, `apply_mono_fn_i64_to_i64_i64`
  - For function types in the signature: flatten as `fn_{params}_to_{ret}`
  - Put in `bust/mono/name_mangler.hpp`

### HIR Cloning

- [ ] Implement deep clone for HIR expression trees
  - Must handle all `ExprKind` variants including `unique_ptr` wrappers
  - Clone needs a type substitution map: replaces `TypeId`s that reference
    polymorphic type variables with concrete `TypeId`s
  - Cloned nodes get new `TypeId`s interned into the same `TypeRegistry`
  - Put in `bust/mono/cloner.hpp`

- [ ] Cloner walks: `Expression` → `ExprKind` variants → `Block` →
  `Statement` → `LetBinding`. Recursively clones `unique_ptr` children.
  Each cloned `Expression` gets its `m_type` substituted.

### Monomorphization Walker

- [ ] `bust/mono/monomorphizer.hpp` / `.cpp`
  - Context: holds the instantiation data, type registry ref, output program
  - Walk each `TopItem`:
    - `FunctionDef` with no free type vars: copy as-is
    - `LetBinding` to a polymorphic lambda: for each recorded instantiation,
      clone the lambda body with the concrete type substitution, create a
      new `LetBinding` with the mangled name
    - `ExternFunctionDeclaration`: copy as-is (never polymorphic)
  - Rewrite identifiers: when an `Identifier` references a polymorphic name,
    replace it with the mangled name for the instantiation that matches
    the identifier's type

- [ ] Handle the "which specialization?" lookup
  - At a call site, the callee `Identifier` has a concrete type after
    type checking. Match that type against the recorded instantiations
    to pick the right mangled name.

- [ ] Entry point: `bust/inc/monomorphizer.hpp`, `bust/src/monomorphizer.cpp`
  - Functor like other passes: `hir::Program operator()(hir::Program)`
  - Wire into `bust.cpp` pipeline between `TypeChecker` and `Zonker`

### Tests

- [ ] Identity function used at one type
  ```
  let id = |x| { x }; id(42)
  ```
  Produces `id_mono_i64`, call site rewritten

- [ ] Identity function used at two types
  ```
  let id = |x| { x }; id(42); id(true)
  ```
  Produces `id_mono_i64` and `id_mono_bool`

- [ ] Higher-order polymorphic function
  ```
  let apply = |f, x| { f(x) };
  let double = |x: i64| { x + x };
  apply(double, 5)
  ```
  `apply` specialized with `f: fn(i64) -> i64, x: i64`

- [ ] Polymorphic function not used (dead code)
  - Should be dropped or left as-is (decide policy)

- [ ] After monomorphization, zonker succeeds (no unresolved type vars)

- [ ] After monomorphization + zonk + codegen, programs produce correct
  results via lli

---

## Phase 3: Integration

- [ ] Wire monomorphization into `bust.cpp` pipeline
- [ ] Add `--dump-mono` flag to dump HIR after monomorphization
- [ ] Verify all existing tests still pass (monomorphization is a no-op
  when there are no polymorphic definitions)
- [ ] Update aspirations.md to check off monomorphization
