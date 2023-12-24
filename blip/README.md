# Blip Language

Blip language based on the chapter 1 toy language from Programming Languages: An Interpreter-Based Approach by Samuel Kamin.

## Implementation

Intial thoughts concerning implementation would be

  - Have a common lexer to read in arbitrary language files and produce a sequence of tokens
    - Token class likely being abstract with various child classes (IntLiteralToken, ParendToken, ...)
  - Blip class would have a method `Parse` that could take a sequence of tokens and return a BlipAST
    - Add method `Eval` to evaluate a BlipAST?
  - BlipAST may be made up of different kinds of ASTNodes (BlipASTNode?)
    - Can also contain methods for pretty printing AST or other useful features
