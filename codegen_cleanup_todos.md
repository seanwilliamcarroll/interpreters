# Closure Rework Plan

Current action plan for the lambda/closure decomposition. See `aspirations.md`
§ "Codegen (LLVM IR)" and § "Codegen Cleanup" for rationale and for remaining
cleanup items not covered here.

## 1. Scaffold `ClosureBuilder`

- [ ] Create `bust/codegen/closure_builder.hpp` + `.cpp`
- [ ] Ctor `(Context&, const std::vector<zir::IdentifierExpr>& captures)` — moves `analyze_captures` logic: intern env type, resolve capture handles into `std::vector<Argument>`
- [ ] `allocate_and_populate_env() -> Handle` (malloc env + store-captures loop, including the alloca-load branch)
- [ ] `emit_capture_load_prologue()` (alloca + load-from-env + store per capture)
- [ ] `package_fat_pointer(GlobalHandle lambda_handle, Handle env_handle) -> Handle`
- [ ] Add to CMake

## 2. Rewrite `LambdaExpr` on with-captures path

- [ ] Branch at top: `if (captures.empty())` → temporary fallback to old code
- [ ] Non-empty branch uses `ClosureBuilder` end-to-end
- [ ] Delete `analyze_captures` (absorbed into `ClosureBuilder` ctor)
- [ ] Keep `generate_lambda_signature` (still used by both paths)
- [ ] Verify existing tests pass

## 3. Lift capture-less lambdas to top-level functions

- [ ] Audit `Module` for a constant-globals mechanism; add one if missing
- [ ] New helper `lift_free_lambda(lambda_expr) -> GlobalHandle` — emits top-level function + `@<name>.closure = constant %closure { ptr @<name>, ptr null }`, no env, no prologue
- [ ] Wire into the `empty()` branch of `LambdaExpr`; remove stub fallback

## 4. Constant closures for top-level functions and externs

- [ ] `TopItemGenerator::operator()(FunctionDef)` emits `@foo.closure` after the function body
- [ ] `TopItemGenerator::operator()(ExternFunctionDeclaration)` emits the closure global pointing to the thunk
- [ ] Audit current `IdentifierExpr` resolution for top-level functions; change it to hand back the address of the closure global instead of freshly malloc'ing a fat pointer
- [ ] Check close `aspirations.md` sub-bullet "Constant closure globals for top-level functions and thunks"

## 5. Verify and clean up

- [ ] Existing test suite passes unchanged (uniform ABI = no test updates)
- [ ] Hand-inspect IR output for one lambda-with-captures and one capture-less case: confirm no-capture path does not malloc
- [ ] Delete any now-unreachable code in `expression_generator.cpp`

## Out of scope (tracked in aspirations.md)

- Stack-allocating fat pointers when closures don't escape
- Direct-call optimization for statically-known callees (Phase 2)
- Refcounting env structs
- `ConstantClosureBuilder` — only if emitting constants grows beyond a one-liner helper
