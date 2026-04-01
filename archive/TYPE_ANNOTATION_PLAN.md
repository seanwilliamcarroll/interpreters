# Type Annotation Plan

Goal: Add optional type annotations to the blip language, laying groundwork for a type checker pass.

## Syntax

```scheme
;; Typed function parameters + return type
(define (add (x : int) (y : int)) : int
  (+ x y))

;; Untyped function (still valid)
(define (add x y) (+ x y))

;; Typed variable
(define x : int 5)

;; Untyped variable (still valid)
(define x 5)
```

Type names: `int`, `double`, `bool`, `string`, `unit`
Function types (for higher-order): deferred for now — start with base types.

## Steps

### Step 1: Lexer — Add COLON token
- [ ] Add `COLON` to `TokenType` enum in `blip_tokens.hpp`
- [ ] Handle `':'` in `lexer.cpp` — single character token, like `(` and `)`
- [ ] Add `COLON` case to `token_type_to_string` in `blip_tokens.cpp`
- [ ] Add lexer tests for the colon token

### Step 2: AST — Add type annotation nodes
- [ ] Create a `TypeNode` AST node (holds a type name string + location)
- [ ] Add `TypeNode` to the visitor interface in `ast_visitor.hpp`
- [ ] Modify `DefineFnNode` to hold optional type annotations per parameter and an optional return type
- [ ] Modify `DefineVarNode` to hold an optional type annotation
- [ ] Update `AstPrinter` to print type annotations

Key design decision: parameters change from bare `Identifier` to a `TypedParam` structure
(or similar) that pairs an `Identifier` with an optional `TypeNode`. This affects `DefineFnNode`
and `parse_identifier_list`.

### Step 3: Parser — Parse type annotations
- [ ] Parse colon + type name after function parameters: `(x : int)`
- [ ] Parse optional return type annotation: `) : int`
- [ ] Parse optional type annotation on variable defines: `(define x : int 5)`
- [ ] Untyped parameters and variables must still work (backwards compatible)
- [ ] Add parser tests for all annotation variants

### Step 4: Evaluator — Pass through annotations (no-op)
- [ ] Add `visit(const TypeNode&)` to evaluator (can be a no-op or unreachable)
- [ ] Ensure all existing evaluator tests still pass
- [ ] The evaluator ignores type annotations — they're for the checker only

### Step 5: Type checker visitor (future)
Not part of this plan — but this is what the annotations enable.

## Notes

- Type annotations are **optional** at every position — this keeps the language backwards compatible
- The parser distinguishes typed params `(x : int)` from untyped `x` by whether
  it sees a LEFT_PAREND or an IDENTIFIER when parsing the parameter list
- `:` only appears inside `define` forms — it's not a general operator
- Base types only for now: `int`, `double`, `bool`, `string`, `unit`
