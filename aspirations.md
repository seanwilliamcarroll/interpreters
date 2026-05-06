# Aspirations

Open work threads, loosely grouped by theme. Items within a group that depend
on each other are marked with arrows (→ means "enables").

## Codegen Cleanup

Three structural investments that unblock future features:

- **Type-property visitor/table.** `width_bits` is already a single dispatched
  function on `LLVMType` (`codegen/types.hpp:68`), but `is_signed_type`,
  `to_llvm_type`, and the `StructType` branch in formatting are still each a
  separate visit. Adding `ArrayType` means editing every site ("shotgun
  surgery"). One `type_traits(ty)` returning `{ signed, width, llvm_name,
  category }` centralizes it.
- **Def/use introspection on instructions.** `Instruction` is a variant of
  POD structs — fine for emission, nothing for analysis. Every optimization
  (DCE, CSE, constant folding) and even a validity checker wants to
  enumerate the handles an instruction defines vs uses. Two free functions
  `result(const Instruction&) -> optional<Handle>` and
  `operands(const Instruction&) -> vector<Handle>` unlock every future pass.
- **Stricter handle typing.** `BlockLabel` (`codegen/block_label.hpp`) is
  already a distinct, builder-constructed type — `BranchInstruction::m_iftrue`
  and `JumpInstruction::m_target` are correctly typed. Remaining narrowing:
  `CallInstruction::m_callee` is still `Value` but must semantically be a
  function pointer or global; tightening it would catch the last class of
  malformed-IR-at-runtime bugs at compile time.

## Aggregate Types

- [ ] Arrays (fixed-size)
- [ ] Strings (likely built on arrays or a dedicated type)

## User-Defined Types

- [ ] Structs (declaration, field access, construction)
- [ ] Enums / tagged unions

## Mutability / References

- [ ] Reject assignment to a captured binding inside a closure body
  — Today, `let mut x = 1; let f = || { x = 99; }; f();` type-checks and
  runs, but bust captures by value, so the closure mutates only its local
  copy of `x` — the outer binding is untouched. Rust catches the analogous
  program (closures that mutate captures are `FnMut` and require `let mut f`
  to call). Conservative fix at HIR type-check: track current lambda depth
  on the context, stamp each binding with the depth it was declared at,
  and reject Assignment whose place's binding has a lower depth than
  current. Stops users from writing programs that look like they should
  mutate the outer but silently don't.
- [ ] References (`&T`, `&mut T`) and Rust-style closure capture
  — Borrow checker, lifetimes, capture-by-`&mut` by default for mutating
  closures, `move` keyword to opt out. Removes the need for the rule
  above — closure mutation just works and propagates to the outer
  binding. Substantial feature on its own; the rule above is the bridge
  in the meantime.

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

- [ ] Multi-file module system for user code
  — `import`/`use` syntax so programs can span files. Requires path syntax,
  visibility rules, cyclic-import detection, and cross-file type resolution.
- [ ] Expand the prelude
  — Curate a set of libc externs (`putchar`, `getchar`, `puts`, `free`, …)
  into `bust/std/prelude.bs`. Mechanism is in place; this is a curation
  decision, not a design one.
- [ ] Core standard library
  — Built on top of the module system; wraps common libc functions and
  provides bust-native utilities.
