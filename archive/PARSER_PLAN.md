# Parser Plan

## Background

The lexer is complete. It consumes raw source text and emits a stream of typed tokens.
The parser's job is to find **structure** in that stream and produce an **Abstract Syntax Tree (AST)**.

Blip's keywords (`IF`, `WHILE`, `SET`, `BEGIN`, `PRINT`, `DEFINE`) and the presence of
paren tokens suggest blip is **s-expression based** — syntax like `(define foo 42)` or
`(if cond (begin ...))`. This simplifies parsing considerably: parens make structure
explicit, so the grammar is unambiguous and easy to recurse over.

---

## Step 1: Write the Grammar

### Informal summary

- An **expression** is either an atom or a parenthesized list
- An **atom** is a literal (int, double, string, bool) or an identifier
- A **list** is `(` followed by one or more expressions, followed by `)`
- Some lists are **special forms** — keywords with fixed, known structure:
  `if`, `while`, `set`, `begin`, `define`, `print`
- Anything else is a **function call** (callee expression + arguments)

### Formal grammar

```
program     ::= expression* EOF

expression  ::= atom
              | '(' list ')'

atom        ::= INT_LITERAL
              | DOUBLE_LITERAL
              | STRING_LITERAL
              | BOOL_LITERAL
              | IDENTIFIER

list        ::= 'if'     expression expression expression?          -- condition, then, else?
              | 'while'  expression expression                       -- condition, body
              | 'set'    IDENTIFIER expression                       -- variable assignment
              | 'begin'  expression+                                 -- sequencing
              | 'print'  expression                                  -- output
              | 'define' IDENTIFIER expression                       -- variable definition
              | 'define' '(' IDENTIFIER IDENTIFIER* ')' expression   -- function definition
              | expression expression*                               -- function call
```

### Design notes

- **`define` is overloaded**: `(define x 42)` for variables, `(define (f x y) body)` for
  functions. The parser distinguishes by peeking after `define` — `(` means function form,
  `IDENTIFIER` means variable form.
- **`if` has optional else**: parser peeks for `)` after the then-branch to decide.
- **`while` takes one body expression**: use `(while cond (begin ...))` for multiple statements.
- **`set` target is a bare `IDENTIFIER`** — no nested lvalues.
- **Function call callee is any expression**, allowing `((get-fn) arg)` — not just identifiers.

---

## Step 2: Design the AST Node Types

Each construct in the language gets a node type. Decide on node types before
implementing anything — they define what the parser is trying to build.

**Leaf nodes (atoms):**
- Integer literal
- String literal
- Bool literal
- Identifier

**Special form nodes:**
- `IfNode` — condition, then-branch, optional else-branch
- `WhileNode` — condition, body
- `SetNode` — name, value expression
- `BeginNode` — ordered list of expressions
- `DefineNode` — name, value expression (or parameter list + body)
- `PrintNode` — one expression

**Other:**
- `CallNode` — callee identifier + argument list
- `ProgramNode` — top-level list of expressions (root of the tree)

**Decision:** Inheritance (`AstNode` base class + subclasses). Familiar, easy to extend,
and consistent with the existing `Token` hierarchy. General-purpose leaf nodes and the base
class live in `core/`; blip-specific special forms live in `blip/`.

`DefineNode` splits into two forms:
- `DefineVarNode` — name + value expression
- `DefineFnNode` — name + parameter list + body expression

---

## Step 3: Connect Parser to Lexer

The parser drives the lexer — it pulls tokens on demand rather than receiving a
pre-built list. Standard interface:

- `peek()` — look at the next token without consuming
- `advance()` — consume and return the current token
- `expect(type)` — consume a token of the expected type, or throw

The `Parser` class should hold a reference to (or own) a `LexerInterface`. This is
what the stub comment in `parser.hpp` is already pointing at.

---

## Step 4: Implement Recursive Descent

Recursive descent is the right strategy here. Each grammar rule becomes a function.
Code mirrors the grammar, which makes it easy to reason about and extend.

**Top-level loop:** call `parse_expression()` until EOF, accumulate into `ProgramNode`.

**`parse_expression()`:**
- Next token is a literal or identifier → return a leaf node
- Next token is `(` → call `parse_list()`

**`parse_list()`:**
- Consume `(`
- Peek at next token:
  - Keyword → dispatch to the appropriate special-form parser
  - Identifier → parse as function call
- Consume `)` at the end

**Each special form** gets its own function with fixed expected structure.
For example, `parse_if()` expects: condition expr, then expr, optional else expr.

---

## Step 5: Error Handling

Use the existing `CompilerException`, which already carries source location.

Cases to handle:
- Expected `(` but got something else
- Unexpected keyword or token type inside a form
- Unexpected EOF mid-expression
- Wrong number of arguments to a special form

Good error messages (pointing at the right line and column) make the language
much easier to debug.

---

## Step 6: Test the Parser

Follow the existing test patterns (DocTest + RapidCheck):

- Test each node type parses correctly from valid input
- Test that malformed input throws with a useful error
- Consider property tests: any expression that the lexer accepts either parses
  successfully or throws a `CompilerException` — never crashes or silently corrupts.

---

## Step 7: Wire into `Blip::rep()`

Once the parser works, update `Blip::rep()` to parse the token stream rather than
just dumping tokens. At this stage, printing the AST (a "pretty printer") is useful
for debugging before the evaluator exists.

---

## Sequencing

```
1. Write the grammar informally
2. Define AST node type headers
3. Give Parser access to the lexer (peek/advance/expect)
4. Implement parse_expression() + parse_list()
5. Implement each special form parser
6. Add error handling throughout
7. Wire into Blip::rep() and test end-to-end
```

The **evaluator** (actually running code) comes after — the parser only builds the tree.
