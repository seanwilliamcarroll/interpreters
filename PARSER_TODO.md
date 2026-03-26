# Parser Implementation TODO

## Infrastructure
- [x] Parser class with peek/advance/expect
- [x] Test scaffolding with parse_string helper
- [x] parse() — top-level loop producing ProgramNode

## Atoms (parse_expression)
- [x] Integer literal
- [x] Double literal
- [x] String literal
- [x] Bool literal
- [x] Identifier
- [x] Error on unexpected token (e.g. `)`)

## Lists (parse_list)
- [ ] Consume `(`, dispatch on first token, consume `)`
- [ ] Function call — `(expr expr*)`

## Special Forms
- [x] print — `(print expr)`
- [ ] set — `(set IDENTIFIER expr)`
- [x] begin — `(begin expr+)`
- [ ] if — `(if expr expr expr?)`
- [ ] while — `(while expr expr)`
- [ ] define variable — `(define IDENTIFIER expr)`
- [ ] define function — `(define (IDENTIFIER IDENTIFIER*) expr)`

## Tests
- [x] Integer, string, bool literals
- [x] Identifier
- [x] Print
- [x] Empty program
- [x] Unexpected token throws
- [x] Double literal
- [ ] Set, begin, if, while, define
- [ ] Function call
- [ ] Nested expressions (e.g. `(print (if true 1 2))`)
- [ ] Define function with params
- [x] Multiple top-level expressions

## Integration
- [ ] Wire parser into Blip::rep()
