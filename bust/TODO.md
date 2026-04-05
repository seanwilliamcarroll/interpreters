# Bust — Next Steps

## Type Checker Fixes
- [x] Verify comparison operators (`<`, `>`, `<=`, `>=`) return `bool`, not operand type
- [x] Forward references: two-pass over top-level items (collect signatures, then check bodies)

## Type Inference
- [ ] Allow lambdas with no type annotations (params and return type)
- [ ] Hindley-Milner type inference and unification
  - Type variables, substitution, `unify(T1, T2)`
  - Generalize let-bindings (let-polymorphism)
  - Constraint generation + solving

## Evaluator
- [ ] Tree-walking interpreter over HIR
  - Value representation (i64, bool, unit, closures)
  - Environment with variable bindings
  - Function calls and recursion
  - Control flow (if/else, return as early exit)

## Backend
- [ ] HIR -> LLVM IR
