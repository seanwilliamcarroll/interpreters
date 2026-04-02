# Bust Grammar

## Terminals

```
// Keywords
FN          = `fn`
LET         = `let`
RETURN      = `return`
IF          = `if`
ELSE        = `else`
WHILE       = `while`
FOR         = `for`
TRUE        = `true`
FALSE       = `false`

// Type keywords
I64         = `i64`
BOOL        = `bool`
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

// Literals and identifiers
NONZERO_DIGIT = '1' | ... | '9'
DIGIT         = '0' | NONZERO_DIGIT
INT_LITERAL   = '0' | NONZERO_DIGIT DIGIT*
IDENTIFIER    = [_a-z][_a-zA-Z0-9]*   // checked against keyword table
```

## Grammar Rules

```
program             = top_item+

top_item            = func_def
                    | let_binding

func_def            = FN IDENTIFIER parameter_list (ARROW type)? block

parameter_list      = LPAREN (argument_annotated (COMMA argument_annotated)*)? RPAREN

argument_annotated  = IDENTIFIER COLON type

argument_inferred   = IDENTIFIER (COLON type)?

type                = I64
                    | BOOL
                    | UNIT

block               = LBRACE expression_or_binding* expression? RBRACE

expression_or_binding
                    = expression SEMICOLON
                    | let_binding

let_binding         = LET IDENTIFIER (COLON type)? EQUALS expression SEMICOLON

// Expression precedence chain (loosest to tightest)

expression          = logic_or

logic_or            = logic_and (OR_OR logic_and)*

logic_and           = equality (AND_AND equality)*

equality            = comparison ((EQ_EQ | BANG_EQ) comparison)*

comparison          = add_sub ((LESS | GREATER | LESS_EQ | GREATER_EQ) add_sub)*

add_sub             = mult_div_mod ((PLUS | MINUS) mult_div_mod)*

mult_div_mod        = unary_pre ((STAR | SLASH | PERCENT) unary_pre)*

unary_pre           = (MINUS | BANG) postfix
                    | postfix

postfix             = primary (LPAREN (expression (COMMA expression)*)? RPAREN)*

primary             = literal
                    | IDENTIFIER
                    | LPAREN expression RPAREN
                    | block
                    | if_expr
                    | while_expr
                    | for_expr
                    | lambda_expr
                    | return_expr

// Compound expressions

return_expr         = RETURN expression

if_expr             = IF expression block (ELSE block)?

lambda_expr         = PIPE (argument_inferred (COMMA argument_inferred)*)? PIPE (ARROW type)? block

while_expr          = TODO (deferred — recursion covers looping for now)
for_expr            = TODO (deferred — needs ranges/collections)

// Literals

literal             = INT_LITERAL
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

- All variables are immutable. Shadowing via `let` is allowed.
- No assignment expression or statement (follows from immutability).
- Consider outlawing non-const global variables.
- `if`/`else` is an expression (returns a value).
- `return expr` is an expression — typechecks `expr` against the enclosing
  function's return type, then the `return_expr` itself is compatible with any
  expected type (acts like a "never" / bottom type without needing to add `!`
  to the type system).
- Implicit return: last expression in a block is its value.

## Desirable Features (Future)

- Hindley-Milner type inference
- Rich type system (starting with i64, bool, ())
- Mutability via `let mut` (if/when needed)
