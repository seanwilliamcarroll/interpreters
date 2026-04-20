# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Codegen Cleanup — HIGH PRIORITY

`bust/codegen` has accumulated enough mess that new features (aggregate types,
optimizations, multi-backend) will be painful without a cleanup pass. Summary
of the issues, grouped by theme. Running checklist lives in
`codegen_cleanup_todos.md`.

### Raw string literals are doing three different jobs

Strings in codegen play three distinct roles that are currently mixed together:

- **LLVM IR syntax tokens** — `" = load "`, `"br i1 "`, `"store "`, `"alloca "`,
  `"getelementptr "`, `"call void "`, `"ret void"`, etc. scattered throughout
  `formatter.cpp`. These are the syntax of the target dialect; a change in
  LLVM version or a move to a different textual form touches dozens of sites.
- **Runtime ABI / convention names** — `"malloc"` (hardcoded in
  `expression_generator.cpp`), `"env"`, `"entry"`, `"main"` (string-compared
  in `top_item_generator.cpp`), `"param_"` prefix, `"lambda"`, `"if_result"`,
  `"short_circuit_logic_result"`, block labels `"then"` / `"else"` / `"merge"`
  / `"rhs"`. These define the closure ABI and naming discipline.
- **IR literal spellings** — `"0"`, `"1"`, `"null"`, `"true"`, `"false"`,
  `"-1"`. These are textual forms of IR constants frozen into C++ strings.

`naming_conventions.hpp` is the right idea but only covers a small slice. The
principle: if a string appears in code, someone has to recognize it and
typos pass the compiler. If it is a named constant, the compiler helps. Beyond
that, grouping constants by role makes the seam between codegen layers
obvious — the formatter should only know about syntax tokens, the lambda
generator should only know about ABI names.

### Confusing logic

- **`IfExpr` generator** interleaves control-flow construction with the
  "does this if-expression produce a value?" decision. The decision is made
  *after* both arms are emitted, which makes the routine hard to read
  top-to-bottom. Natural shape: decide yields-value up front, allocate the
  result slot if so, emit each arm with its own store+jump, emit the merge
  with a load.
- **`LambdaExpr` generator** is ~120 lines mixing ~7 concerns: scope
  management, signature creation, env-struct typing, capture-load prologue
  (inside the new function), capture-store (at the creation site in the
  outer function), body generation, and fat-pointer packaging.
- **`FunctionScopeGuard`** is defined mid-way through `expression_generator.cpp`
  and papers over the fact that `Module::current_function` is a single
  mutable raw pointer when it semantically wants to be a stack (entering a
  lambda mid-function).
- **`get_block_type`** is a method on `ExpressionGenerator` but needs nothing
  from that class; it is a pure query over `zir::Block` that landed on the
  wrong owner.
- **`malloc_struct` hardcodes `GlobalHandle{"malloc"}`**, coupling codegen
  to the C ABI. Allocator choice should be configurable via `Context` or a
  dedicated runtime-ABI module.

### Repeated structure

- **Void/non-void instruction variants.** `CallVoidInstruction` vs
  `CallInstruction`, `ReturnVoidInstruction` vs `ReturnInstruction`. The
  split causes three+ near-identical emission sites (`CallExpr`,
  `ExternFunctionDeclaration` thunk, `FunctionDef` terminator). LLVM treats
  a void call as a call with no result name — collapsing to
  `std::optional<Handle> m_target` removes a whole axis of branching.
- **`alloca + store + … + load`** appears in the if-merge, the short-circuit
  merge, capture-load prologue, and let-bindings. That is a high-level
  operation ("materialize a value through memory") without a name.
- **Comma-separated printing** (`function_parameters`, `function_arguments`)
  is the same loop written twice.
- **`std::visit(m_handle_converter, X)`** appears ~30 times across the
  formatter; a one-liner helper makes it vanish.
- **Signed/unsigned compare switch** (`to_llvm_compare_condition`) has four
  near-identical pairs of cases. A data table keyed by
  `(BinaryOperator, signed?)` is cleaner and extends more easily to new
  numeric types.

### The missing abstraction — an IR builder

Most sites in `ExpressionGenerator` follow a four-step incantation: mint a
temp, construct the instruction struct with designated initializers, append
to the current block, return the temp. Every caller knows the insertion
point, the SSA-temp allocator, and the struct layout — a Law of Demeter
violation. Callers reach through `m_ctx.function().current_basic_block()`.

The right abstraction is a stateful construction helper in the style of
`llvm::IRBuilder`: an object that owns the insertion point, the SSA counter,
and provides high-level `create_*` methods. What it unlocks:

- **One-line instruction emission** — `create_load(src, ty)` replaces the
  four-step pattern.
- **Scoped insertion-point management** — `auto _ = builder.at(block)` RAII
  restores the previous insertion point. Replaces the handwritten
  `FunctionScopeGuard` and the explicit `set_insertion_point` calls in
  `IfExpr`, `BinaryExpr` short-circuit, and `LambdaExpr`.
- **Natural home for high-level helpers** — `store_to_struct` /
  `load_from_struct` / `malloc_struct` already exist but live on
  `ExpressionGenerator` (wrong owner). They belong on the builder, with the
  allocator symbol configurable.
- **Composition point for domain builders** — `ClosureBuilder` wraps
  `IRBuilder` to hide env-struct creation, malloc, capture stores, and fat
  pointer packaging. The 120-line lambda method shrinks to a dozen lines of
  choreography.

Tradeoff: a stateful builder concentrates mutable state, so it's easier to
use and harder to reason about under concurrency. For a single-threaded
codegen it's pure upside.

### Paving the way for future work

Three structural investments that unblock future features:

- **Type-property visitor/table.** `is_signed_type`, `width_bits`,
  `to_llvm_type`, and the `StructType` branch in formatting are each a
  separate visit over `zir::Type` / `LLVMType`. Adding `ArrayType` means
  editing every site ("shotgun surgery"). One `type_traits(ty)` returning
  `{ signed, width, llvm_name, category }` centralizes it.
- **Def/use introspection on instructions.** `Instruction` is a variant of
  POD structs — fine for emission, nothing for analysis. Every optimization
  (DCE, CSE, constant folding) and even a validity checker wants to
  enumerate the handles an instruction defines vs uses. Two free functions
  `result(const Instruction&) -> optional<Handle>` and
  `operands(const Instruction&) -> vector<Handle>` unlock every future pass.
- **Stricter handle typing.** `BranchInstruction::m_iftrue` is typed
  `Handle` but must semantically be a block label. `CallInstruction::m_callee`
  must be a function pointer or global. A distinct `BlockLabel` type (and
  similar narrowing) makes illegal IR unrepresentable — bugs caught at
  compile time instead of producing malformed IR that LLVM rejects.

### Suggested attack order

1. Extract string constants by role (cheap, mechanical, immediate cleanup).
2. Collapse void/non-void instruction duplication via `optional<Handle>`.
3. Introduce the `IRBuilder` and migrate `ExpressionGenerator` method by
   method — biggest structural win.
4. Pull struct/malloc helpers onto the builder; build `ClosureBuilder` on
   top; shrink `LambdaExpr`.
5. Add `operands()` / `result()` for instructions. Prerequisite for passes.
6. Tighten handle types at branch targets and call callees.

## Type System

- [x] Collapse unified type variables to their root before generalization
  — When `x + y` unifies `?T<0>` and `?T<1>`, the unifier knows they're
  the same type but stores them as two entries pointing to the same root.
  Before generalization decides which variables are polymorphic, substitute
  all unified variables with their root representative so the system sees
  one variable, not two aliases. Without this, monomorphization would try
  to substitute two independent type parameters when there's really one.
  Prerequisite for monomorphization (see Pipeline Overhaul).

## New Types and FFI

- [x] char, i8, i32, and cast expressions (landed)
- [x] Extern function declarations (syntax, parsing, type checking, codegen)
- [x] `putchar` via libc

  Extern declarations → putchar

## Aggregate Types

- [ ] Arrays (fixed-size)
- [ ] Tuples
- [ ] Strings (likely built on arrays or a dedicated type)

## User-Defined Types

- [ ] Structs (declaration, field access, construction)
- [ ] Enums / tagged unions

## Codegen (LLVM IR)

- [ ] Lambda/closure codegen
  - [x] Uniform calling convention (`ptr %env` first param on all user functions except main)
  - [x] Extern thunks (`.thunk` wrappers so externs can be used as first-class values)
  - [x] Env type creation and loading captures in lambda bodies
  - [x] Env allocation (malloc) and storing captures at lambda creation sites
  - [x] Fat pointer construction (`{fn_ptr, env_ptr}`) for closure values
  - [x] Indirect calls through closure values (extract fn_ptr + env_ptr)
  - [ ] Constant closure globals for top-level functions and thunks (`@foo.closure = constant %closure { ptr @foo, ptr null }`)
- [x] Non-i64 integer types in codegen
- [x] Cast expressions in codegen
- [ ] Codegen type arena
  — Unify `LLVMType` enum and `TypeHandle` into a single type representation.
  `CodegenTypeId` indexes into an arena of `CodegenType = variant<PrimitiveType, StructType>`.
  `StructType` holds a name + list of `CodegenTypeId` fields. Arena pre-registers
  primitives (i1, i8, i32, i64, ptr, void). Subsumes the type-registry role of
  `CaptureEnv` in `Module`. Parallels the ZIR type arena design.
- [ ] Optimizations (LLVM pass pipeline, inlining, etc.)

## Control Flow / Return Analysis

- [ ] Smarter reasoning about `return` expressions in blocks
  — Currently, `return expr;` with a trailing semicolon makes it a statement,
  so the block's final expression is absent and the block type is unit. This
  means `fn foo() -> i64 { return 42; }` is a type error because the block
  returns `()`, not `i64`. Rust handles this by special-casing return/diverging
  statements. We should either: (a) treat `return` as a diverging expression
  (type `!`) so the block type is still valid, or (b) allow semicolons after
  the final expression in a block without forcing the block type to unit.
  Either approach would make `return x + y;` legal in tail position.

## Type Unifier

- [ ] Make `TypeUnifier::find` const via `mutable` on the union-find internals
  — Path compression mutates the parent pointers during `find`, which is why
  it currently can't be `const`. This is the textbook use case for `mutable`:
  the logical result is unchanged, only an internal cache is rewritten.
  Marking the union-find storage `mutable` lets const callers (e.g. a
  post-type-check resolution pass) invoke `find` while preserving the
  path-compression optimization.

## Pipeline Overhaul

Goal: polymorphic lambdas work end-to-end through codegen.

- [x] Drop evaluator (remove from CMake build, evaluator no longer maintained)
- [x] Monomorphization pass
  - Sits between type checking and zonking
  - Canonicalization → monomorphization → polymorphic lambdas resolve cleanly
- [ ] ZIR: arena-based zonked intermediate representation
  - New representation produced by the zonker, replacing reuse of `hir::Program`
  - Expression arena (flat `ExprId` indexing instead of `unique_ptr<Expr>`)
  - Concrete types only, no `UnifierState`
  - Codegen reads ZIR instead of HIR
- [ ] Codegen targets ZIR
  - Migrate codegen to consume ZIR instead of `hir::Program`

  Drop evaluator → monomorphization → ZIR → codegen targets ZIR

  The HIR (with `unique_ptr` trees) stays as-is for type checking and
  monomorphization. The zonker becomes a lowering pass from HIR to the
  arena-backed ZIR. Read-only passes after zonking (codegen, future
  optimizations) work against the arena.

## Memory Management

- [ ] Reference counting for heap-allocated closures (env structs)
  — Currently leaking malloc'd env structs. Refcounting is sufficient since
  no mutation means no cycles; tracing GC is overkill for now.

## Module System / Standard Library

- [ ] Module or include system
  — So users don't have to manually declare externs like `putchar` and `malloc`.
  Could be an implicit prelude, explicit `import`/`use` syntax, or both.
- [ ] Core standard library
  — Built on top of the module system; wraps common libc functions and
  provides bust-native utilities.
