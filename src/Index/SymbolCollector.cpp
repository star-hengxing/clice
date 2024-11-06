#include "SymbolBuilder.h"

namespace clice::index {

namespace {

class SymbolCollector : public clang::RecursiveASTVisitor<SymbolCollector> {
public:
    using Base = clang::RecursiveASTVisitor<SymbolCollector>;

    SymbolCollector(SymbolBuilder& builder, clang::ASTContext& context) :
        builder(builder), context(context), srcMgr(context.getSourceManager()) {}

public:
    /// ============================================================================
    ///                                Declaration
    /// ============================================================================

    VISIT_DECL(NamespaceDecl) {
        /// `namespace Foo { }`
        ///             ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(NamespaceAliasDecl) {
        /// `namespace Foo = Bar`
        ///             ^     ^~~~ reference
        ///             ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        if(auto location = builder.addLocation(decl->getTargetNameLoc())) {
            auto symbol = builder.addSymbol(decl->getNamespace());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    VISIT_DECL(UsingDirectiveDecl) {
        /// `using namespace Foo`
        ///                   ^~~~~~~ reference
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl->getNominatedNamespace());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    VISIT_DECL(LabelDecl) {
        /// `label:`
        ///    ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(TagDecl) {
        /// `struct/class/union/enum Foo { };`
        ///                           ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(FieldDecl) {
        /// `struct Foo { int bar; };`
        ///                    ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
            symbol.addTypeDefinition(decl->getType());
        }
        return true;
    }

    VISIT_DECL(EnumConstantDecl) {
        /// `enum Foo { bar = 1 };`
        ///              ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
            symbol.addTypeDefinition(decl->getType());
        }
        return true;
    }

    VISIT_DECL(TypedefNameDecl) {
        /// `typedef int Foo;`
        ///               ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
            symbol.addTypeDefinition(decl->getUnderlyingType());
        }
        return true;
    }

    VISIT_DECL(VarDecl) {
        /// `int foo;`
        ///       ^~~~ declaration/definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDeclarationOrDefinition(decl->isThisDeclarationADefinition(), location);
            symbol.addTypeDefinition(decl->getType());
        }
        return true;
    }

    VISIT_DECL(FunctionDecl) {
        /// `void foo();`
        ///         ^~~~ declaration/definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDeclarationOrDefinition(decl->isThisDeclarationADefinition(), location);

            if(auto CMD = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
                // FIXME: virtual override const ...

                if(auto CCD = llvm::dyn_cast<clang::CXXConstructorDecl>(CMD)) {
                    symbol.addTypeDefinition(CCD->getThisType());
                }

                // FIXME: CXXConversionDecl, CXXDestructorDecl
            }

            // FIXME: CXXDeductionGuideDecl
        }
        currentFunction = decl;
        return true;
    }

    /// FIXME: how to resolve shadow decls? a name could refer to multiple decls.
    /// Don't need to handle UsingEnumDecl, it's handled by VisitTypeLoc.
    VISIT_DECL(UsingDecl) {
        return true;
    }

    VISIT_DECL(BindingDecl) {
        /// `auto [a, b] = std::make_tuple(1, 2);`
        ///        ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
            symbol.addTypeDefinition(decl->getType());
        }
        return true;
    }

    VISIT_DECL(TemplateTypeParmDecl) {
        /// `template <typename T> ...`
        ///                     ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(TemplateTemplateParmDecl) {
        /// `template <template <typename> class T> ...`
        ///                                      ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(NonTypeTemplateParmDecl) {
        /// `template <int N> ...`
        ///                ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
            symbol.addTypeDefinition(decl->getType());
        }
        return true;
    }

    VISIT_DECL(ClassTemplateDecl) {
        /// `template <typename T> class Foo { };`
        ///                               ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDeclarationOrDefinition(decl->isThisDeclarationADefinition(), location);
        }
        return true;
    }

    VISIT_DECL(FunctionTemplateDecl) {
        /// `template <typename T> void foo();`
        ///                               ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDeclarationOrDefinition(decl->isThisDeclarationADefinition(), location);
        }
        return true;
    }

    VISIT_DECL(TypeAliasTemplateDecl) {
        /// `template <typename T> using Foo = T;`
        ///                               ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    VISIT_DECL(VarTemplateDecl) {
        /// `template <typename T> int foo;`
        ///                             ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDeclarationOrDefinition(decl->isThisDeclarationADefinition(), location);
        }
        return true;
    }

    VISIT_DECL(ConceptDecl) {
        /// `template <typename T> concept Foo = ...;`
        ///                           ^~~~ definition
        if(auto location = builder.addLocation(decl->getLocation())) {
            auto symbol = builder.addSymbol(decl);
            symbol.addOccurrence(location);
            symbol.addDefinition(location);
        }
        return true;
    }

    /// ============================================================================
    ///                                  TypeLoc
    /// ============================================================================

    /// We don't care about type without location information.
    constexpr bool TraverseType [[gnu::const]] (clang::QualType) {
        return true;
    }

    VISIT_TYPELOC(TagTypeLoc) {
        /// `struct Foo { }; Foo foo`
        ///                   ^~~~ reference
        if(auto location = builder.addLocation(loc.getNameLoc())) {
            auto symbol = builder.addSymbol(loc.getDecl());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    VISIT_TYPELOC(TypedefTypeLoc) {
        /// `using Foo = int; Foo foo`
        ///                    ^~~~ reference
        if(auto location = builder.addLocation(loc.getNameLoc())) {
            auto symbol = builder.addSymbol(loc.getTypedefNameDecl());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    VISIT_TYPELOC(TemplateSpecializationTypeLoc) {
        /// `std::vector<int> foo`
        ///        ^~~~ reference
        if(auto location = builder.addLocation(loc.getTemplateNameLoc())) {
            auto symbol = builder.addSymbol(declForType(loc.getType()));
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    /// ============================================================================
    ///                                 Statement
    /// ============================================================================

    VISIT_EXPR(DeclRefExpr) {
        /// `foo = 1`
        ///   ^~~~ reference
        if(auto location = builder.addLocation(expr->getLocation())) {
            auto symbol = builder.addSymbol(expr->getDecl());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    VISIT_EXPR(MemberExpr) {
        /// `foo.bar`
        ///       ^~~~ reference
        if(auto location = builder.addLocation(expr->getMemberLoc())) {
            auto symbol = builder.addSymbol(expr->getMemberDecl());
            symbol.addOccurrence(location);
            symbol.addReference(location);
        }
        return true;
    }

    // FIXME: modify template resolver to cache recursively resolved results.
    VISIT_EXPR(DependentScopeDeclRefExpr) {
        return true;
    }

    VISIT_EXPR(CXXDependentScopeMemberExpr) {
        return true;
    }

    VISIT_EXPR(CallExpr) {
        return true;
    }

    /// FIXME:
    constexpr bool TraverseNestedNameSpecifier [[gnu::const]] (clang::NestedNameSpecifier*) {
        return true;
    }

    bool TraverseNestedNameSpecifierLoc(clang::NestedNameSpecifierLoc NNS) {
        // FIXME: use TemplateResolver here.
        auto range = NNS.getSourceRange();
        auto range2 = NNS.getLocalSourceRange();
        return Base::TraverseNestedNameSpecifierLoc(NNS);
    }

    bool TraverseTemplateArgumentLoc(const clang::TemplateArgumentLoc& argument) {
        return Base::TraverseTemplateArgumentLoc(argument);
    }

    bool TraverseCXXBaseSpecifier(const clang::CXXBaseSpecifier& base) {
        return Base::TraverseCXXBaseSpecifier(base);
    }

    bool TraverseConstructorInitializer(clang::CXXCtorInitializer* init) {
        return Base::TraverseConstructorInitializer(init);
    }

private:
    SymbolBuilder& builder;
    clang::ASTContext& context;
    clang::SourceManager& srcMgr;

    const clang::FunctionDecl* currentFunction = nullptr;
};

void sortSymbols(std::vector<memory::Symbol>& symbols, std::vector<Occurrence>& occurrences) {
    const auto size = symbols.size();
    std::vector<uint32_t> indices;
    indices.reserve(size);
    for(std::size_t index = 0; index < size; index++) {
        indices.emplace_back(index);
    }

    llvm::sort(indices, [&](uint32_t lhs, uint32_t rhs) {
        return symbols[lhs].id < symbols[rhs].id;
    });

    std::vector<uint32_t> map;
    map.resize(size);
    for(std::size_t index = 0; index < size; index++) {
        /// original index -> new index
        map[indices[index]] = index;
    }

    /// Adjust the index in occurrences and relations.
    for(auto& occurrence: occurrences) {
        occurrence.symbol.offset = map[occurrence.symbol.offset];
    }

    for(auto& symbol: symbols) {
        for(auto& relation: symbol.relations) {
            // if(Relation::isSymbol(relation)) {
            //     relation.symOrLoc = map[relation.symOrLoc];
            // }
        }
    }

    /// Sort symbols.
    llvm::sort(symbols, [](const memory::Symbol& lhs, const memory::Symbol& rhs) {
        return lhs.id < rhs.id;
    });
}

void sortFiles(std::vector<memory::File>& files, std::vector<Location>& locations) {
    const auto size = files.size();
    std::vector<uint32_t> indices;
    indices.reserve(size);
    for(std::size_t index = 0; index < size; index++) {
        indices.emplace_back(index);
    }

    llvm::sort(indices, [&](uint32_t lhs, uint32_t rhs) {
        return files[lhs].path < files[rhs].path;
    });

    std::vector<uint32_t> map;
    map.resize(size);
    for(std::size_t index = 0; index < size; index++) {
        /// original index -> new index
        map[indices[index]] = index;
    }

    /// Adjust the index in files and .
    for(auto& location: locations) {
        if(location.file.isValid()) {
            location.file = FileRef{map[location.file.offset]};
        }
    }

    /// Sort files.
    llvm::sort(files, [](const memory::File& lhs, const memory::File& rhs) {
        return lhs.path < rhs.path;
    });
}

void SymbolBuilder::indexTU(memory::Index& result, Compiler& compiler) {
    index = &result;
    SymbolCollector collector(*this, compiler.tu()->getASTContext());
    collector.TraverseAST(compiler.context());

    /// Beacuse we store the index of symbols in Occurrence and Relation, Sorting symbols will
    /// invalidate the index. So we need to adjust the them. First, we need to obtain the index
    /// mapping before and after sorting.

    sortFiles(result.files, result.locations);
    sortSymbols(result.symbols, result.occurrences);

    llvm::sort(result.occurrences, [&](const Occurrence& lhs, const Occurrence& rhs) {
        if(lhs.location.isInvalid() || rhs.location.isInvalid()) {
            std::terminate();
        }
        return result.locations[lhs.location.offset] < result.locations[rhs.location.offset];
    });
}

}  // namespace

memory::Index index(Compiler& compiler) {
    memory::Index result;
    SymbolBuilder builder(compiler.sema(), compiler.tokBuf());
    builder.indexTU(result, compiler);
    return std::move(result);
};  // namespace clice::index

}  // namespace clice::index