# Evaluator Plan

## Background

The parser is complete. It produces an AST from source text. The next step is to make
the tree *do something* — evaluate it. This requires two new pieces:

1. **Environment** — a data structure that maps names to values (variable/function bindings)
2. **Evaluator** — a visitor that walks the AST and computes results

Together they turn blip from a parser into an interpreter.

---

## Concepts

### What is an Environment?

An environment is a **chain of scopes**. Each scope is a map from names to values.
When you look up a variable, you search the innermost scope first, then walk outward
toward the global scope. This is called **lexical scoping** (or static scoping) — a
variable refers to the binding in the closest enclosing scope where it was defined.

Why a chain, not a single flat map?

- `(define x 1)` at the top level lives in the **global** scope
- `(define (f x) (+ x 1))` — the parameter `x` lives in a **local** scope created
  for each call to `f`, and shadows the global `x`
- When `f` returns, its local scope is destroyed — the global `x` is unaffected

Each scope holds a pointer to its **parent**. Lookup walks the chain; definition
writes to the current (innermost) scope. `set` mutates an *existing* binding by
walking the chain to find it.

### What is a Value?

The evaluator needs a runtime representation for the result of evaluating any
expression. This is distinct from the AST — an `IntLiteral` node *evaluates to*
an integer value, but a `CallNode` could evaluate to any type depending on the
function.

Blip values are:
- **int** — from `IntLiteral`
- **double** — from `DoubleLiteral`
- **bool** — from `BoolLiteral`
- **string** — from `StringLiteral`
- **function** — from `DefineFnNode` (a closure: parameter list + body + captured environment)
- **unit/void** — the result of forms that don't produce a meaningful value (`print`, `while`, `set`)

`std::variant` is a natural fit here — it's a type-safe tagged union that makes it
impossible to accidentally treat an int as a string.

### How does the Evaluator work?

The evaluator is an `AstVisitor`. Each `visit()` method evaluates its node and
stores the result. The key insight: **evaluation is recursive**. Evaluating
`(+ 1 2)` means: evaluate `+` (look it up → get a function), evaluate `1` (→ int 1),
evaluate `2` (→ int 2), then call the function with those arguments.

The visitor's `visit()` methods return void (that's how the interface is defined),
so the evaluator stores its result in a member variable (e.g. `m_result`) that gets
overwritten on each visit. This is the same pattern as `AstPrinter` using `m_out`.

---

## Steps

### Step 1: Define the Value type

- [ ] Create `blip/inc/value.hpp`
- [ ] Define `Value` as a `std::variant<int, double, bool, std::string, Function, Unit>`
- [ ] `Function` holds: parameter names, body pointer, captured environment pointer
- [ ] `Unit` is an empty struct (the "no meaningful value" type)
- [ ] Add a `value_to_string()` helper for printing values

**Why first:** Everything else depends on knowing what a Value is. The evaluator
produces Values, the environment stores Values.

### Step 2: Define the Environment

- [ ] Create `blip/inc/environment.hpp`
- [ ] `Environment` holds a `std::unordered_map<std::string, Value>` and a pointer to a parent `Environment`
- [ ] `define(name, value)` — binds name in the *current* scope (error if already defined in this scope)
- [ ] `lookup(name)` → walks the chain, returns value (throws if not found)
- [ ] `set(name, value)` → walks the chain, mutates existing binding (throws if not found)

**Design question — shared_ptr vs unique_ptr:**
Closures capture their defining environment. If a function is returned or stored,
that environment must outlive the scope that created it. This means environments
need shared ownership → `std::shared_ptr<Environment>`. This is standard for
tree-walking interpreters.

### Step 3: Build the Evaluator visitor (literals + identifiers only)

- [ ] Create `blip/inc/evaluator.hpp` and `blip/src/evaluator.cpp`
- [ ] `Evaluator` implements `AstVisitor`, holds a `shared_ptr<Environment>` and a `Value m_result`
- [ ] Literals: each `visit(const IntLiteral&)` etc. just stores the literal's value in `m_result`
- [ ] `visit(const Identifier&)` does `m_result = environment->lookup(name)`
- [ ] `visit(const ProgramNode&)` evaluates each child expression in sequence
- [ ] Wire into `Blip::rep()` — parse, then evaluate, then print the result
- [ ] Tests for literal evaluation and identifier lookup

**Why start here:** This is the simplest possible evaluator. It proves the
Value/Environment/Visitor wiring works before adding any complexity.

### Step 4: Add special forms (define, set, begin, print, if, while)

- [ ] `visit(DefineVarNode)` — evaluate the value expression, bind in current environment
- [ ] `visit(DefineFnNode)` — create a Function value capturing current environment
- [ ] `visit(SetNode)` — evaluate value, call `environment->set()`
- [ ] `visit(BeginNode)` — evaluate each expression in sequence, result is the last one
- [ ] `visit(PrintNode)` — evaluate expression, print via `value_to_string()`, result is Unit
- [ ] `visit(IfNode)` — evaluate condition, check truthiness, evaluate appropriate branch
- [ ] `visit(WhileNode)` — loop: evaluate condition, if truthy evaluate body, repeat
- [ ] Tests for each form

**Truthiness decision:** Need to decide what counts as "true". Options:
- Only `bool true` is truthy (strict)
- `false` and `0` are falsy, everything else truthy (C-style)
- Only `bool` is allowed in conditions (type error otherwise)

### Step 5: Function calls

- [ ] `visit(CallNode)` — evaluate callee, evaluate arguments, then call:
  1. Verify callee is a `Function` value
  2. Create a new environment with the closure's captured environment as parent
  3. Bind each parameter name to the corresponding argument value
  4. Evaluate the function body in that new environment
  5. Result is what the body evaluated to
- [ ] Arity checking — error if argument count doesn't match parameter count
- [ ] Tests: basic calls, closures, recursion

**This is where closures come alive.** A function like:
```
(define (make-adder n) (define (add x) (+ n x)) add)
```
When `make-adder` is called, it creates `add` in a local scope where `n` is bound.
The returned `add` function captures that scope, so `n` persists even after
`make-adder` returns.

### Step 6: Built-in functions

- [ ] Add a `BuiltinFunction` variant to Value (or a separate callable type)
- [ ] Register arithmetic: `+`, `-`, `*`, `/`
- [ ] Register comparisons: `<`, `>`, `=`
- [ ] Populate the global environment with built-ins at startup
- [ ] Tests

**Without built-ins, the language can't do arithmetic.** These are functions that
exist in the global scope but are implemented in C++ rather than in blip source.

---

## Open Questions

- **Error handling strategy:** Should runtime errors (type mismatch, unbound variable,
  wrong arity) use `CompilerException` with a "RuntimeError" phase, or a new exception type?
- **Truthiness:** Strict (bool-only) vs. permissive (C-style)?
- **Print destination:** Should `print` write to `std::cout` or an injectable `std::ostream&`?
  Injectable is better for testing.
- **REPL behavior:** Should the REPL print the result of every top-level expression,
  or only when `print` is used explicitly?
