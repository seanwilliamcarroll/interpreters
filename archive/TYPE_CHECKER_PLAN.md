# Type Checker Plan

Goal: Build a type checker visitor that validates types before evaluation, catching errors at "compile time" rather than runtime.

## What we have

- Type annotations on function params: `(define (f (x : int) (y : int)) : int ...)`
- Return type annotations (mandatory) on functions
- Variable defines infer type from initializer: `(define x 5)` — checker figures out `int`
- `TypeNode` on the AST, `Identifier::get_type()` for params, `DefineFnNode::get_return_type()` for return types
- Built-in functions with known signatures (+, -, *, /, <, >, =)

## Type representation

```
enum class Type { Int, Double, Bool, String, Unit, Fn }
```

- `Fn` is an **opaque** function type — the checker knows it's callable but doesn't know the signature
- Calls to `Fn`-typed values are unchecked (argument types and return type are trusted)
- Calls to *known* functions (defined with full signatures in scope) are fully checked
- Later, `Fn` can be upgraded to carry signature info (e.g. `(fn (int int) int)`) to close this hole

## Design decisions

- **Strict typing, no implicit promotion** — `(+ 1 2.0)` is a type error. Both operands must be the same numeric type. A cast operation can be added later.
- **If/else branches must have the same type** — `(if true 1 2.0)` is a type error.
- **`set` cannot change a variable's type** — the checker enforces this.
- **Type checker is a separate pass** — runs after parsing, before evaluation. Another `AstVisitor`.
- **The checker computes types, not values** — stores `Type m_result` instead of `Value m_result`.
- **Variable type annotations are checked assertions** — `(define x : int 5)` verifies the initializer is `int`. Without annotation, the type is inferred from the initializer.

## Steps

### Step 1: Type enum and type environment
- [ ] Create a `Type` enum: `Int`, `Double`, `Bool`, `String`, `Unit`, `Fn`
- [ ] Create `type_to_string` for error messages
- [ ] Create a string-to-Type mapping (to convert `TypeNode` string names to the enum)
- [ ] Create a `TypeEnvironment` class — same scope chain as `Environment`, but maps names to `Type`
- [ ] Register built-in function types in a default type environment

### Step 2: Type checker visitor (literals + variables)
- [ ] Create `TypeChecker` class implementing `AstVisitor`
- [ ] Literals return their obvious type (IntLiteral → Int, etc.)
- [ ] `DefineVarNode`: infer type from initializer, bind name → type. If annotation present, verify it matches.
- [ ] `Identifier`: look up type in the type environment
- [ ] `SetNode`: check that the new value's type matches the variable's existing type (no type changes)
- [ ] `ProgramNode` / `BeginNode`: check children sequentially, result type is last child's type

### Step 3: Type checker (control flow)
- [ ] `IfNode`: condition must be `Bool`, then/else branches must have identical types (if else exists). If no else, result is `Unit`.
- [ ] `WhileNode`: condition must be `Bool`, result is `Unit`
- [ ] `PrintNode`: any type is printable, result is `Unit`

### Step 4: Type checker (functions)
- [ ] `DefineFnNode`: create child type environment, bind param names to declared types, check body type matches declared return type, bind function name to `Fn` in parent env
- [ ] `CallNode` for known functions: look up callee type, if it has a known signature (built-ins), check argument count and types, result is the known return type
- [ ] `CallNode` for `Fn`-typed values: callee is `Fn`, skip argument/return type checking (opaque)

### Step 5: Integration and testing
- [ ] Wire the type checker into the pipeline: parse → **check** → evaluate
- [ ] Type errors produce `CompilerException` with source location
- [ ] All existing tests still pass (they are well-typed programs)
- [ ] Add tests for type errors: wrong types in arithmetic, if branches don't match, set changes type, wrong argument types to known functions, etc.
