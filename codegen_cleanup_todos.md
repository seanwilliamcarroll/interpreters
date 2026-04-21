# Codegen Cleanup — Concrete Steps

Follow-ups from `aspirations.md` § "Codegen Cleanup". Each section is a
self-contained cleanup. Steps within a section are meant to be doable in
roughly one sitting.

## 1. Move `get_block_type` off `ExpressionGenerator`

Current home: `ExpressionGenerator::get_block_type(const zir::Block&)` in
`expression_generator.cpp:111`. Reads only the ZIR arena; nothing class-specific.

- [ ] Pick a new home (free function in `zir/` over the arena, or free
      function in a codegen helper header)
- [ ] Move the body; update the single call site at
      `expression_generator.cpp:121`
- [ ] Delete the declaration from `expression_generator.hpp:36`
- [ ] Rebuild and run `ctest --preset mp -R bust.codegen`

## 2. Make the allocator symbol configurable

Current home: `IRBuilder::malloc_struct` in `ir_builder.cpp:193`. Calls
`GlobalHandle{conventions::allocator_function}` — name is a constant but
still hardcoded to `"malloc"`.

- [ ] Decide the surface: field on `Context` (e.g. `m_allocator_symbol`) vs
      dedicated `RuntimeABI` struct carried by `Context`
- [ ] Populate default (`"malloc"`) where `Context` is constructed
- [ ] Have `malloc_struct` read the allocator symbol from `Context` instead
      of `conventions::`
- [ ] Remove `conventions::allocator_function` if no other caller remains
- [ ] Rebuild and confirm tests still pass

## 3. Extract a comma-separated print helper

`Formatter::function_parameters` (`formatter.cpp:144`) and
`Formatter::function_arguments` (`formatter.cpp:322`) are the same loop
twice: empty-check + all-but-last with `", "` + last without.

- [ ] Add a private template helper on `Formatter` that takes a range and
      a per-element `format` callable (or member pointer)
- [ ] Rewrite `function_parameters` to call it
- [ ] Rewrite `function_arguments` to call it
- [ ] Consider inlining the helper at the two call sites if the template
      hurts readability more than the duplication did

## 4. Replace the compare-condition switch with a data table

Current: `to_llvm_compare_condition` in `expression_generator.cpp:251`, a
switch with eight cases — EQ/NE plus four signed/unsigned pairs (LT, LE,
GT, GE).

- [ ] Define the table — options to weigh:
      (a) `constexpr std::array` of `{op, signed, condition}` triples,
      (b) two small maps keyed by `BinaryOperator` (signed vs unsigned),
      (c) keep the switch but de-duplicate via a pair `(signed_cond, unsigned_cond)`
      lookup per `op`
- [ ] Replace the body of `to_llvm_compare_condition` with the chosen form
- [ ] Verify `bust.codegen.expressions` still passes (the comparison suite
      exercises all eight cases)

## 5. Name the "materialize-a-value-through-memory" pattern

Today the pattern `alloca + store-per-branch + load-at-join` is open-coded
in:

- `IfExpr` merge (`expression_generator.cpp:~130–173`)
- short-circuit `&&` / `||` merge (`expression_generator.cpp:~345–381`)

(`capture-load prologue` and let-bindings use `alloca + store` but loads
happen elsewhere — different pattern, leave for now.)

- [ ] Prototype an abstraction on `IRBuilder`. Two candidate shapes:
      (a) a `BranchMerge` RAII helper: `auto merge = builder.begin_merge(type);
          ... merge.store_in_arm(value); ... auto result = merge.finalize();`,
      (b) a `create_phi_via_memory(type, std::span<BranchEmitter>)` free
          call where each emitter owns its block's body
- [ ] Migrate `IfExpr` to the new abstraction; keep short-circuit on the
      old path to compare readability
- [ ] If the migration reads well, migrate short-circuit too; otherwise
      revert and document why
- [ ] Remove any now-unused helpers from `IRBuilder`

## Suggested order

Ordered by cost and independence. Each step is committable on its own.

1. Step 1 (`get_block_type`) — trivially mechanical, warms up the touch points.
2. Step 3 (comma-separated helper) — localized to `formatter.cpp`.
3. Step 4 (compare-condition table) — localized to `expression_generator.cpp`.
4. Step 2 (allocator configuration) — touches `Context` but still narrow.
5. Step 5 (branch-merge abstraction) — needs design thought; biggest reach.
