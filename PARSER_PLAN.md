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

Before any code, write down the grammar informally. For an s-expression language:

- An **expression** is either an atom or a list
- An **atom** is a literal (int, string, bool) or an identifier
- A **list** is `(` followed by one or more expressions, followed by `)`
- Some lists are **special forms** — keywords with fixed, known structure:
  `if`, `while`, `set`, `begin`, `define`, `print`
- Anything else is a **function call** (identifier + arguments)

Write this out precisely in English before writing code. It forces clear thinking.

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

**Key design question:** one class hierarchy or `std::variant`?

| Approach | Pros | Cons |
|---|---|---|
| Inheritance (`AstNode` base + subclasses) | Familiar OOP, easy to add new node types | Visiting requires virtual dispatch or Visitor pattern |
| `std::variant` | Exhaustive matching — compiler flags missed cases | Adding node types requires editing the variant everywhere |

Inheritance is the more familiar starting point. `std::variant` is worth knowing
about as a more modern alternative.

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
