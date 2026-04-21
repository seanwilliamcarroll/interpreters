# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Codegen Cleanup

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

## Aggregate Types

- [ ] Arrays (fixed-size)
- [ ] Tuples
- [ ] Strings (likely built on arrays or a dedicated type)

## User-Defined Types

- [ ] Structs (declaration, field access, construction)
- [ ] Enums / tagged unions

## Codegen (LLVM IR)

- [ ] Direct calls for statically-known callees (skip fat pointer entirely)
  — When a `CallExpr`'s callee is syntactically a top-level function or
  extern (not a value captured into a local), emit `call @foo(null, args...)`
  instead of loading `{fn_ptr, env_ptr}` from the closure struct and
  doing an indirect call. Dispatch is syntactic (walk the callee expression
  before lowering), not based on Handle variant or type. Also eliminates
  the thunk hop for direct extern calls (`putchar(c)` → `call @putchar(c)`
  instead of `call @putchar.thunk(null, c)`). Pure optimization atop the
  constant-closure ABI — does not change fat-pointer representation.
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

## Memory Management

- [ ] Reference counting for heap-allocated closures (env structs)
  — Currently leaking malloc'd env structs. Refcounting is sufficient since
  no mutation means no cycles; tracing GC is overkill for now.
- [ ] Stack-allocate the fat-pointer struct when the closure doesn't escape
  — Today `package_fat_pointer` mallocs the `{fn_ptr, env_ptr}` struct even
  when the closure is consumed immediately by a local call. If escape
  analysis can prove the fat pointer doesn't outlive its creation scope,
  an `alloca` is free. Recognizing when this is safe is itself interesting
  (prerequisite: some form of use-site tracking or an escape analysis pass).

## Module System / Standard Library

- [ ] Module or include system
  — So users don't have to manually declare externs like `putchar` and `malloc`.
  Could be an implicit prelude, explicit `import`/`use` syntax, or both.
- [ ] Core standard library
  — Built on top of the module system; wraps common libc functions and
  provides bust-native utilities.
