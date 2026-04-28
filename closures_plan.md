# Closures Through the Type System

## Problem

Codegen dispatches "direct call" vs "fat-pointer call" using binding kind
(`FunctionBinding` / `ClosureBinding` / `AllocaBinding`) plus a syntactic peek
at the let-binding RHS. This breaks for `let add5 = make_adder(5);` — the RHS
is a `CallExpr` returning a closure, the AST shape gives no hint, dispatch
falls back to direct-call, and the env is dropped on the floor.

Root cause: zir has one `FunctionType` covering both shapes. The FnPtr-vs-Closure
distinction never makes it across call / return / let boundaries.

## Approach

Split function-shape in zir, and add a trait-like type so higher-order code
can be written uniformly:

- `FnPtrType { params, return }` — bare code pointer. No env, no allocation.
  Top-level `fn` and free (non-capturing) lambdas.
- `ClosureType { params, return }` — fat pointer `{fn, env}`. Capturing
  lambdas, and anything that may return a capturing lambda.
- `CallableType { params, return }` — trait-like; both of the above are
  subtypes. HOF parameters and returns that need to accept either shape.

Calling convention at codegen: `CallableType` always uses the closure ABI;
`FnPtrType` values flowing into a `CallableType` slot are auto-promoted to
`{fn, null_env}`. `lift_free_lambda` stays — it's the `FnPtrType` emission
path.

## Tasks

### zir
- [ ] Split `FunctionType` in `bust/zir/types.hpp` into `FnPtrType` and `ClosureType`
- [ ] Add `CallableType` and subsumption rules (`FnPtrType <: CallableType`, `ClosureType <: CallableType`)
- [ ] Update `Type` variant, hashing, and `operator<=>` for the new types
- [ ] Decide surface syntax: keep `fn(T) -> U` for `FnPtrType`; pick a spelling for `CallableType` (e.g. `Fn(T) -> U`)

### Lowerer / type-checker
- [ ] Lambda lowering picks `FnPtrType` when free-variable set is empty, else `ClosureType`
- [ ] Top-level `fn` always lowers to `FnPtrType`
- [ ] Reject capturing body in a `fn(...) -> ...` return-type slot
- [ ] Insert FnPtr → Closure coercion where a `FnPtrType` value flows into a `CallableType` slot (argument, return, let-binding)

### Codegen
- [ ] CallExpr dispatch keys off the callee's zir TypeId, not binding kind:
  - `FnPtrType` → direct call
  - `ClosureType` / `CallableType` → load `{fn, env}`, indirect call with env first
- [ ] Remove the syntactic RHS-peek in `let_binding_generator`
- [ ] Reconsider `ClosureBinding` — if dispatch is fully type-driven, `AllocaBinding` + `FunctionBinding` may suffice
- [ ] Emit FnPtr → Closure wrapping at coercion sites
- [ ] Keep `lift_free_lambda` as the `FnPtrType` emission path

### Tests
- [ ] Re-enable `make_adder returns a capturing closure` in `bust/test/src/codegen_lambdas_test.cpp`
- [ ] Closure stored in a let-binding, then called
- [ ] HOF taking `fn(T) -> U` vs HOF taking `Fn(T) -> U`
- [ ] Returning a non-capturing lambda (must stay `FnPtrType`, no env alloc)

## Open questions

- [ ] Is `CallableType` a real first-class type, or just a coercion rule between `FnPtrType` and `ClosureType` at boundaries?
- [ ] Surface syntax — borrow Rust's `impl Fn` / `dyn Fn`, or a single `Fn(T) -> U`?
- [ ] Do we need `FnMut` / `FnOnce`, or is one `Fn` enough until ownership exists?
