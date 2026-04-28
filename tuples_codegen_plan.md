# Tuples in Codegen

Finish PR #44. AST → parser → HIR → zir → type checker → monomorpher all
shipped. Codegen has two stub operators and no execution tests. Type lowering
(`zir::TupleType` → `StructType`) already works in `context.hpp:92`, anonymous
struct types already render inline (`arena.hpp:40-52`), and heap struct ops
already exist on `IRBuilder` (`malloc_struct`, `store_to_struct`,
`load_from_struct`).

## Storage strategy

Stack alloca for tuple values. Construction stores fields into the alloca;
projection GEPs to a field and loads. Function-return loads the whole struct
out of the alloca and `ret`s it as an SSA struct value; caller stores the
returned SSA struct into its own alloca.

## Tasks

- [ ] Add stack-alloca-of-struct helper on `IRBuilder` (parallel to `malloc_struct` but using `emit_alloca`)
- [ ] Verify `create_load` works for whole-struct loads (LLVM allows it; just confirm the existing helper takes a `StructType` `TypeId` cleanly)
- [ ] Implement `ExpressionGenerator::operator()(const zir::TupleExpr &)` at `bust/codegen/expression_generator.cpp:82`
- [ ] Implement `ExpressionGenerator::operator()(const zir::DotExpr &)` at `bust/codegen/expression_generator.cpp:618`
- [ ] Verify let-binding path: rhs is an SSA struct value, must be stored into the binding's alloca
- [ ] Verify `IdentifierExpr` load path for tuple-typed bindings produces the SSA struct value expected by `DotExpr` and `ret`
- [ ] Verify call-site path: a tuple-returning call yields an SSA struct value usable by the next consumer
- [ ] Add `bust/test/src/codegen_tuples_test.cpp` and wire into the test CMake target
- [ ] Test: 2-tuple construction + `t.0` / `t.1` projection
- [ ] Test: heterogeneous tuple `(i64, bool)`
- [ ] Test: nested tuple + chained projection `t.0.1`
- [ ] Test: tuple bound to `let`, projected later
- [ ] Test: tuple passed as function argument
- [ ] Test: tuple returned from function, projected at call site
- [ ] Test: tuple returned from function, bound to `let` then projected
- [ ] Test: projection result used in arithmetic / control flow
- [ ] Run `ctest --preset mp -R bust.codegen` and confirm all tuple tests pass
- [ ] Remove tuples from the `Aggregate Types` list in `aspirations.md`
