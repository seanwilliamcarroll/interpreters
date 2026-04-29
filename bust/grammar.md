# Bust Grammar

## Terminals

```
// Keywords
EXTERN      = `extern`
FN          = `fn`
LET         = `let`
MUT         = `mut`
RETURN      = `return`
IF          = `if`
ELSE        = `else`
WHILE       = `while`
FOR         = `for`
TRUE        = `true`
FALSE       = `false`

// Type keywords
I8          = `i8`
I32         = `i32`
I64         = `i64`
BOOL        = `bool`
CHAR        = `char`
UNIT        = `()`

// Delimiters
LPAREN      = `(`
RPAREN      = `)`
LBRACE      = `{`
RBRACE      = `}`

// Punctuation
ARROW       = `->`
COLON       = `:`
SEMICOLON   = `;`
COMMA       = `,`
DOT         = `.`
EQUALS      = `=`
PIPE        = `|`

// Arithmetic operators
PLUS        = `+`
MINUS       = `-`
STAR        = `*`
SLASH       = `/`
PERCENT     = `%`

// Comparison operators
EQ_EQ       = `==`
BANG_EQ     = `!=`
LESS        = `<`
GREATER     = `>`
LESS_EQ     = `<=`
GREATER_EQ  = `>=`

// Logical operators
AND_AND     = `&&`
OR_OR       = `||`
BANG        = `!`

// Comments (consumed by lexer, not passed to parser)
LINE_COMMENT  = `//` (any char except newline)* newline
BLOCK_COMMENT = `/*` (any char, including newlines)* `*/`  // nesting allowed

// Cast operator
AS          = `as`

// Literals and identifiers
NONZERO_DIGIT   = '1' | ... | '9'
DIGIT           = '0' | NONZERO_DIGIT
HEX_DIGIT       = DIGIT | 'a' | ... | 'f' | 'A' | ... | 'F'
INT_LITERAL     = '0' | NONZERO_DIGIT DIGIT*
PRINTABLE_CHAR  = any ASCII 32-126 except "'" and "\"
ESCAPE_SEQ      = '\' ('n' | 't' | 'r' | '\\' | '\'' | '0')
                | '\x' HEX_DIGIT HEX_DIGIT
CHAR_LITERAL    = "'" (PRINTABLE_CHAR | ESCAPE_SEQ) "'"
IDENTIFIER      = [_a-z][_a-zA-Z0-9]*   // checked against keyword table
```

## Grammar Rules

```
program             = top_item+

top_item            = func_def
                    | extern_func_declaration

func_def            = FN IDENTIFIER parameter_list (ARROW type)? block

extern_func_declaration = EXTERN FN IDENTIFIER parameter_list (ARROW type)? SEMICOLON

parameter_list      = LPAREN (argument_annotated (COMMA argument_annotated)*)? RPAREN

argument_annotated  = IDENTIFIER COLON type

argument_inferred   = IDENTIFIER (COLON type)?

type                = I8
                    | I32
                    | I64
                    | BOOL
                    | CHAR
                    | UNIT
                    | function_type
                    | tuple_type
                    | LPAREN type RPAREN

function_type       = FN LPAREN (type (COMMA type)*)? RPAREN ARROW type

// A tuple type requires at least one COMMA to disambiguate from a
// parenthesized type. `(i64)` is just `i64`; `(i64,)` is a 1-tuple.
// The 0-tuple `()` is spelled UNIT (lexed as a single token).
tuple_type          = LPAREN type COMMA RPAREN
                    | LPAREN type (COMMA type)+ COMMA? RPAREN

block               = LBRACE expression_or_binding* expression? RBRACE

expression_or_binding
                    = expression_no_semicolon
                    | expression SEMICOLON
                    | let_binding
                    | assignment

// These expressions end with a block, so they don't need a trailing semicolon
// to be used as statements. A semicolon is still allowed but not required.
expression_no_semicolon
                    = if_expr
                    | block
                    | while_expr
                    | for_expr

let_binding         = LET MUT? IDENTIFIER (COLON type)? EQUALS expression SEMICOLON

// Assignment is a statement, not an expression. The LHS is a `place` —
// currently just an IDENTIFIER, but the nonterminal exists so that future
// place forms (tuple projection, struct field, deref) can extend it without
// changing the assignment rule.
assignment          = place EQUALS expression SEMICOLON

place               = IDENTIFIER

// Expression precedence chain (loosest to tightest)

expression          = logic_or

logic_or            = logic_and (OR_OR logic_and)*

logic_and           = equality (AND_AND equality)*

equality            = comparison ((EQ_EQ | BANG_EQ) comparison)*

comparison          = add_sub ((LESS | GREATER | LESS_EQ | GREATER_EQ) add_sub)*

add_sub             = mult_div_mod ((PLUS | MINUS) mult_div_mod)*

mult_div_mod        = unary_pre ((STAR | SLASH | PERCENT) unary_pre)*

unary_pre           = (MINUS | BANG) cast_expr
                    | cast_expr

cast_expr           = postfix (AS type)*

postfix             = primary (call_suffix | dot_suffix)*

call_suffix         = LPAREN (expression (COMMA expression)*)? RPAREN

// Tuple projection. When structs land, extend to `DOT (INT_LITERAL | IDENTIFIER)`.
dot_suffix          = DOT tuple_index

tuple_index         = INT_LITERAL

primary             = literal
                    | IDENTIFIER
                    | paren_or_tuple_expr
                    | block
                    | if_expr
                    | while_expr
                    | for_expr
                    | lambda_expr
                    | return_expr

// `(expr)` is a parenthesized expression; `(expr,)` is a 1-tuple;
// `(expr, expr, ...)` (with optional trailing comma) is an N-tuple.
// The 0-tuple `()` is the UNIT literal (lexed as a single token).
paren_or_tuple_expr = LPAREN expression RPAREN
                    | LPAREN expression COMMA RPAREN
                    | LPAREN expression (COMMA expression)+ COMMA? RPAREN

// Compound expressions

return_expr         = RETURN expression

if_expr             = IF expression block (ELSE block)?

lambda_expr         = PIPE (argument_inferred (COMMA argument_inferred)*)? PIPE (ARROW type)? block

while_expr          = TODO (deferred — recursion covers looping for now)
for_expr            = TODO (deferred — needs ranges/collections)

// Literals

literal             = INT_LITERAL
                    | CHAR_LITERAL
                    | TRUE
                    | FALSE
                    | UNIT
```

## Semantic Requirements

- Exactly one `func_def` must have the name `main`.
- `main` must return `i64`.
- Omitting `-> type` on a `func_def` implies `-> ()`.
- A block with no trailing expression implicitly evaluates to `()`.

## Design Notes

- Local bindings are immutable by default; `let mut` opts in to mutation.
  Shadowing via `let` is still allowed regardless of `mut`.
- Assignment (`x = expr;`) is a statement, not an expression — no
  `if (x = 5)`-style usage. Type of an assignment statement is `()`.
- `let` bindings only appear inside blocks. Top-level globals are a separate
  feature (deferred), and will be spelled `const` / `static` (à la Rust)
  rather than reusing `let`. This is a change from earlier — top-level
  `let` was previously accepted; that path is being removed.
- The LHS of an assignment is a *place expression*. v0 restricts places to
  bare identifiers; tuple projection, struct fields, and deref will extend
  the `place` nonterminal later.
- `if`/`else` is an expression (returns a value).
- `return expr` is an expression — typechecks `expr` against the enclosing
  function's return type, then the `return_expr` itself is compatible with any
  expected type (acts like a "never" / bottom type without needing to add `!`
  to the type system).
- Implicit return: last expression in a block is its value.

## Desirable Features (Future)

- Hindley-Milner type inference
- Rich type system (starting with i64, bool, ())
