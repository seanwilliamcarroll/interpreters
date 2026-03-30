# Refactor Plan: Consolidate core into blip

Related: GitHub issue #12

## Goal

Move the bulk of the code into blip, leaving core as a minimal foundation. Also:
- Decouple TokenType from the Token class — the current extension pattern (enum continuing from `END_TOKEN + 1`) is fragile
- Rename `sc::` namespace to `core::` and flatten `core/inc/sc/` to `core/inc/`

## What stays in core (`core::`)

- `SourceLocation` — filename, line, column tracking
- `LexerInterface<TT>` — abstract interface templated on token type, returns `Token<TT>`
- `Token<TT>` / `TokenOf<TT, V>` — class templates parameterized on a token type enum

All templates — core is a header-only library. Any language instantiates with its own `enum class`.

## Namespace and directory changes

- `sc::` → `core::` everywhere
- `core/inc/sc/*.hpp` → `core/inc/*.hpp` (flatten the directory)
- All `#include <sc/foo.hpp>` → `#include <foo.hpp>` (or whatever the new path is)

## What moves to blip (`blip::`)

| Currently in core | Destination |
|---|---|
| `CoreLexer` (core_lexer.cpp) | blip/src — it's the only lexer we have, no need for the factory indirection |
| `make_lexer()` factory | Remove — blip constructs the lexer directly |
| AST base nodes: `AstNode`, `AbstractLiteral`, `IntLiteral`, `DoubleLiteral`, `StringLiteral`, `BoolLiteral`, `Identifier`, `ProgramNode`, `CallNode` | blip/inc — merge with blip's AST nodes into one file |
| `AstVisitor` | blip/inc — merge with `BlipAstVisitor` into one visitor |
| `CompilerException` | blip/inc — only blip throws these |
| `token_type_to_string` (core) | blip — merge with blip's version |
| Core token type constants (`EOF_TOKENTYPE`, `LEFT_PAREND`, etc.) | See TokenType redesign below |

## What gets deleted from core

- `sc/ast.hpp`, `sc/ast_visitor.hpp`, `sc/exceptions.hpp`
- `core_lexer.cpp`, `exceptions.cpp`
- `sc/sc.hpp` forward declarations file
- The `sc/` directory entirely

## TokenType redesign

**Current problem:** `TokenType` is `using TokenType = unsigned int`. Token defines an enum of common types, BlipToken continues numbering from `END_TOKEN + 1`. Adding a core token shifts all blip values.

**Approach:** Parameterize `Token` on the token type. Core defines class templates, blip instantiates with its own `enum class`:

```cpp
// core/inc/token.hpp
namespace core {

template <typename TT>
class Token {
public:
  Token(const SourceLocation &loc, TT type);
  TT get_token_type() const;
  const SourceLocation &get_location() const;
private:
  SourceLocation m_loc;
  TT m_type;
};

template <typename TT, typename V>
class TokenOf : public Token<TT> { /* value wrapper */ };

} // namespace core
```

```cpp
// core/inc/lexer_interface.hpp
namespace core {

template <typename TT>
struct LexerInterface {
  virtual ~LexerInterface() = default;
  virtual std::unique_ptr<Token<TT>> get_next_token() = 0;
};

} // namespace core
```

```cpp
// blip/inc/blip_tokens.hpp
namespace blip {

enum class TokenType : unsigned int {
  // Structural
  EOF_TOKEN = 0,
  LEFT_PAREND,
  RIGHT_PAREND,
  // Literals
  IDENTIFIER,
  INT_LITERAL,
  STRING_LITERAL,
  DOUBLE_LITERAL,
  BOOL_LITERAL,
  // Keywords
  IF,
  WHILE,
  SET,
  BEGIN,
  PRINT,
  DEFINE,
};

// Convenient aliases
using Token = core::Token<TokenType>;
using StringToken = core::TokenOf<TokenType, std::string>;
using IntToken = core::TokenOf<TokenType, int>;
using DoubleToken = core::TokenOf<TokenType, double>;
using BoolToken = core::TokenOf<TokenType, bool>;
using LexerInterface = core::LexerInterface<TokenType>;

} // namespace blip
```

**Benefits:**
- Core stays genuinely reusable — any language instantiates with its own enum
- Blip gets full `enum class` type safety — exhaustiveness warnings in switch statements
- All token types defined in one place, no fragile numbering across libraries
- Core becomes header-only (no .cpp files for Token)

## Migration steps

- [ ] 1. Create new branch off main
- [ ] 2. Inline SourceLocation (constructor, operators) into header, delete source_location.cpp
- [ ] 3. Rename `sc::` → `core::`, flatten `core/inc/sc/` → `core/inc/`
- [ ] 4. Templatize `Token`, `TokenOf`, and `LexerInterface` on token type; make core header-only
- [ ] 5. Define `enum class TokenType` in blip with all token types; add type aliases
- [ ] 6. Move `CoreLexer` implementation from core to blip, have it implement `core::LexerInterface<blip::TokenType>`
- [ ] 7. Move AST nodes — merge into `blip/inc/ast.hpp`, merge visitors into one
- [ ] 8. Move `CompilerException` to blip
- [ ] 9. Move `token_type_to_string` — merge into one function in blip
- [ ] 10. Update core's CMakeLists — INTERFACE library (headers only)
- [ ] 11. Update blip's CMakeLists — add moved sources
- [ ] 12. Move lexer tests from core to blip
- [ ] 13. Update all includes and namespaces
- [ ] 14. Verify build and all tests pass

## Risks / things to watch

- **Core becomes header-only**: All templates, SourceLocation inlined. Core's CMake target becomes an INTERFACE library (headers only, no compiled sources).
- **Test infrastructure**: `core_test_lib.hpp` has RapidCheck generators for Token and SourceLocation. Token generators reference specific token type values so they move to blip. SourceLocation generators can stay in core if we keep core tests, or just move everything to blip.
- **Include paths**: lots of `#include <sc/...>` change. This is the bulk of the mechanical work.
- **Template instantiation**: Token methods currently live in `token.cpp` with explicit template instantiations. With templates in headers, these go away — the compiler instantiates as needed. Simpler, but larger headers.
