# Type Checker Plan

Goal: Build a type checker visitor that validates types before evaluation, catching errors at "compile time" rather than runtime.

## What we have

- Type annotations on function params: `(define (f (x : int) (y : int)) : int ...)`
- Return type annotations (mandatory) on functions
- Variable defines infer type from initializer: `(define x 5)` — checker figures out `int`
- `TypeNode` on the AST, `Identifier::get_type()` for params, `DefineFnNode::get_return_type()` for return types
- Built-in functions with known signatures (+, -, *, /, <, >, =)

## Type representation

Need an internal type enum (not strings) for the checker to work with:

```
enum class Type { Int, Double, Bool, String, Unit, Function }
```

Function types need param types + return type. Could be a struct wrapping the enum plus a vector of param types and a return type for the Function case. Or start simple — just the base types, and handle function types as a stretch goal.

## Steps

### Step 1: Type enum and type environment
- [ ] Create a `Type` enum for base types: `Int`, `Double`, `Bool`, `String`, `Unit`
- [ ] Create a string-to-Type mapping (to convert `TypeNode` names to the enum)
- [ ] Create a `TypeEnvironment` class — same scope chain pattern as `Environment`, but maps names to `Type` instead of `Value`
- [ ] Register built-in function types in a default type environment

### Step 2: Type checker visitor (literals + variables)
- [ ] Create `TypeChecker` class implementing `AstVisitor`
- [ ] Literals return their obvious type (IntLiteral → Int, etc.)
- [ ] `DefineVarNode`: infer type from the initializer expression, bind name → type
- [ ] `Identifier`: look up type in the type environment
- [ ] `SetNode`: check that the new value's type matches the variable's existing type
- [ ] `ProgramNode` / `BeginNode`: check children sequentially, result is last child's type

### Step 3: Type checker (control flow)
- [ ] `IfNode`: condition must be Bool, then/else branches must have compatible types
- [ ] `WhileNode`: condition must be Bool, body type doesn't matter (result is Unit)
- [ ] `PrintNode`: any type is printable, result is Unit

### Step 4: Type checker (functions)
- [ ] `DefineFnNode`: create a child type environment, bind param names to their declared types, check body type matches declared return type
- [ ] `CallNode`: look up callee's type, verify it's a function, check argument types match parameter types, result type is the function's return type
- [ ] Handle built-in function calls (known signatures)

### Step 5: Integration
- [ ] Wire the type checker into the pipeline: parse → **check** → evaluate
- [ ] Type errors should produce `CompilerException` with location info
- [ ] All existing tests should still pass (they are well-typed programs)
- [ ] Add tests for type errors that should be caught

## Design decisions

- **Type checker is a separate pass** — runs after parsing, before evaluation. It's another `AstVisitor`, structurally similar to the evaluator.
- **The checker computes types, not values** — where the evaluator stores `Value m_result`, the checker stores `Type m_result` (or similar).
- **`set` cannot change a variable's type** — the checker enforces this, which is a new restriction vs the current evaluator.
- **Function types are a stretch goal** — for now, higher-order functions (passing functions as arguments) won't be fully type-checked. We can represent them as an opaque `Function` type or defer this.
- **Numeric promotion** — need to decide: does `(+ 1 2.0)` type-check? If so, the checker needs the same int/double promotion rules as the evaluator. Suggestion: yes, and the result type is `Double` when either operand is `Double`.

## Open questions

- If/else branch types: must they be identical? Or is it OK for one to be Int and the other Double (with promotion)?
- Should we support optional type annotations on `define` variables as a checked assertion? e.g., `(define x : int 5)` — the checker verifies 5 is int. Already parsed, just needs checking.
