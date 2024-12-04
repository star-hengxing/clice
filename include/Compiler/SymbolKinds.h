#include "Utility.h"
#include "Support/Enum.h"

namespace clice {

namespace impl {

/// In the LSP, there are several different kinds, such as `SemanticTokenType`,
/// `CompletionItemKind`, and `SymbolKind`. Unfortunately, these kinds do not cover all the semantic
/// information we need. It's also inconsistent that some kinds exist in one category but not in
/// another, for example, `Namespace` is present in `SemanticTokenType` but not in
/// `CompletionItemKind`. To address this, we define our own `SymbolKind`, which will be used
/// consistently across our responses to the client and in the index. Users who prefer to stick to
/// standard LSP kinds can map our `SymbolKind` to the corresponding LSP kinds through
/// configuration.
enum class SymbolKind {
    Invalid = 0,
    Comment,         ///< C/C++ comments.
    Number,          ///< C/C++ number literal.
    Character,       ///< C/C++ character literal.
    String,          ///< C/C++ string literal.
    Keyword,         ///< C/C++ keyword.
    Directive,       ///< C/C++ preprocessor directive, e.g. `#include`.
    Header,          ///< C/C++ header name, e.g. `<iostream>` and `"foo.h"`.
    Module,          ///< C++20 module name.
    Macro,           ///< C/C++ macro.
    MacroParameter,  ///< C/C++ macro parameter.
    Namespace,       ///> C++ namespace.
    Class,           ///> C/C++ class.
    Struct,          ///> C/C++ struct.
    Union,           ///> C/C++ union.
    Enum,            ///> C/C++ enum.
    Type,            ///> C/C++ type alias and C++ template type parameter.
    Field,           ///> C/C++ field.
    EnumMember,      ///> C/C++ enum member.
    Function,        ///> C/C++ function.
    Method,          ///> C++ method.
    Variable,        ///> C/C++ variable, includes C++17 structured bindings.
    Parameter,       ///> C/C++ parameter.
    Label,           ///> C/C++ label.
    Concept,         ///> C++20 concept.
    Attribute,       ///> GNU/MSVC/C++11/C23 attribute.
    Operator,        ///> C/C++ operator.
    Paren,           ///> `(` and `)`.
    Bracket,         ///> `[` and `]`.
    Brace,           ///> `{` and `}`.
    Angle,           ///> `<` and `>`.
};

}  // namespace impl

struct SymbolKind {
    static SymbolKind from(const clang::tok::TokenKind kind);

    static SymbolKind from(const clang::Decl::Kind kind);
};

enum class SymbolModifierKind {

};

}  // namespace clice