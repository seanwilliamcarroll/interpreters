# Codegen Cleanup TODOs

Check off as completed. See `aspirations.md` § "Codegen Cleanup" for rationale.

## Extract string constants

- [x] Create `ir_syntax.hpp` with LLVM IR keyword constants (`load`, `store`, `alloca`, `getelementptr`, `br`, `ret`, `call`, `icmp`, `ptrtoint`, etc.)
- [x] Create `ir_literals.hpp` with LLVM literal spellings (`null`, `true`, `false`, `-1`, `0`, `1`)
- [x] Expand `naming_conventions.hpp` to cover all ABI/convention names
- [x] Move `"env"` out of `Context::env_parameter()`
- [x] Move `"entry"` out of `Function` constructor
- [ ] Move `"malloc"` out of `ExpressionGenerator::malloc_struct` (make allocator configurable on `Context`)
- [x] Move block-label strings (`"then"`, `"else"`, `"merge"`, `"rhs"`) into naming conventions
- [x] Move synthetic local names (`"if_result"`, `"short_circuit_logic_result"`, `"lambda"`, `"param_"`) into naming conventions
- [ ] Replace `binding.m_name == "main"` string compare with a typed/named predicate
- [x] Migrate `formatter.cpp` to use the new syntax/literal headers

## Collapse void/non-void instruction variants

- [ ] Replace `CallInstruction` + `CallVoidInstruction` with a single `CallInstruction { optional<Handle> m_target; ... }`
- [ ] Replace `ReturnInstruction` + `ReturnVoidInstruction` with a single `ReturnInstruction { optional<Handle> m_value; ... }`
- [ ] Update formatter to branch on `has_value()` instead of variant case
- [ ] Update emission sites in `ExpressionGenerator`, `TopItemGenerator`, `LetBindingGenerator`

## IR builder abstraction

- [ ] Design `IRBuilder` owning `Context&`, insertion-point pointer, SSA counter
- [ ] Add scoped insertion-point guard (`builder.at(block)` RAII)
- [ ] Add `create_load` / `create_store` / `create_alloca` / `create_binary` / `create_unary` / `create_icmp` / `create_cast` / `create_gep` / `create_call` / `create_branch` / `create_jump` / `create_return`
- [ ] Move `malloc_struct`, `store_to_struct`, `load_from_struct` from `ExpressionGenerator` onto `IRBuilder`
- [ ] Migrate `ExpressionGenerator::operator()` methods to use the builder
- [ ] Migrate `LetBindingGenerator`, `StatementGenerator`, `TopItemGenerator` to use the builder
- [ ] Delete `FunctionScopeGuard` (replaced by builder's scoped insertion point)

## Lambda / closure decomposition

- [ ] Extract `emit_capture_load_prologue` from `LambdaExpr` generator
- [ ] Extract `emit_capture_store_at_creation_site` from `LambdaExpr` generator
- [ ] Extract `package_as_fat_pointer` from `LambdaExpr` generator
- [ ] Extract env-struct type creation into its own helper
- [ ] Build `ClosureBuilder` on top of `IRBuilder`
- [ ] Rewrite `operator()(const zir::LambdaExpr &)` on top of `ClosureBuilder`

## If-expression cleanup

- [ ] Decide yields-value up front (hoist the `get_block_type` check)
- [ ] Split into `emit_if_value` and `emit_if_statement` paths
- [ ] Relocate `get_block_type` off `ExpressionGenerator` (free function or `zir::` helper)

## Module / function-stack correctness

- [ ] Replace `Module::current_function` raw pointer with a proper stack
- [ ] Make "enter function" / "leave function" explicit on `Module`

## Formatter cleanup

- [ ] Add `str(const Handle&)` helper to replace repeated `std::visit(m_handle_converter, X)`
- [ ] Extract comma-separated printer to replace `function_parameters` / `function_arguments` duplication
- [ ] Consider RAII line builder (indent on construct, newline on destruct)
- [ ] Move the `.closure = constant ...` emission into a dedicated `declare_closure_global` method

## Type-property centralization

- [ ] Build single `type_traits(LLVMType)` returning `{ signed, width, llvm_name, category }`
- [ ] Replace `is_signed_type`, `width_bits`, ad-hoc `to_string` branches with calls to the traits function
- [ ] Replace `to_llvm_compare_condition` switch with a data table keyed by `(BinaryOperator, signed)`

## Def/use introspection on instructions

- [ ] Implement `operands(const Instruction&) -> vector<Handle>`
- [ ] Implement `result(const Instruction&) -> optional<Handle>`
- [ ] Write first pass that consumes them (e.g. SSA-validity checker)

## Handle type narrowing

- [ ] Introduce `BlockLabel` as a distinct type (not `Handle`)
- [ ] Retype `BranchInstruction::m_iftrue` / `m_iffalse` / `JumpInstruction::m_target` as `BlockLabel`
- [ ] Tighten `CallInstruction::m_callee` to a function-pointer-or-global type
- [ ] Audit `Handle` variant for other spots where a narrower type applies
