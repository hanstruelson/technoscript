#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Forward declarations for module state handlers
inline void handleStateNoneIM(ParserContext& ctx, char c);
inline void handleStateNoneIMP(ParserContext& ctx, char c);
inline void handleStateNoneIMPO(ParserContext& ctx, char c);
inline void handleStateNoneIMPOR(ParserContext& ctx, char c);
inline void handleStateNoneIMPORT(ParserContext& ctx, char c);
inline void handleStateImportSpecifiersStart(ParserContext& ctx, char c);
inline void handleStateImportSpecifierName(ParserContext& ctx, char c);
inline void handleStateImportSpecifierAs(ParserContext& ctx, char c);
inline void handleStateImportSpecifierLocalName(ParserContext& ctx, char c);
inline void handleStateImportSpecifierSeparator(ParserContext& ctx, char c);
inline void handleStateImportSpecifiersEnd(ParserContext& ctx, char c);
inline void handleStateImportFrom(ParserContext& ctx, char c);
inline void handleStateImportSourceStart(ParserContext& ctx, char c);
inline void handleStateImportSource(ParserContext& ctx, char c);
inline void handleStateImportSourceEnd(ParserContext& ctx, char c);
inline void handleStateNoneEX(ParserContext& ctx, char c);
inline void handleStateNoneEXP(ParserContext& ctx, char c);
inline void handleStateNoneEXPO(ParserContext& ctx, char c);
inline void handleStateNoneEXPOR(ParserContext& ctx, char c);
inline void handleStateNoneEXPORT(ParserContext& ctx, char c);
inline void handleStateExportSpecifiersStart(ParserContext& ctx, char c);
inline void handleStateExportSpecifierName(ParserContext& ctx, char c);
inline void handleStateExportSpecifierAs(ParserContext& ctx, char c);
inline void handleStateExportSpecifierExportedName(ParserContext& ctx, char c);
inline void handleStateExportSpecifierSeparator(ParserContext& ctx, char c);
inline void handleStateExportSpecifiersEnd(ParserContext& ctx, char c);
inline void handleStateExportFrom(ParserContext& ctx, char c);
inline void handleStateExportSourceStart(ParserContext& ctx, char c);
inline void handleStateExportSource(ParserContext& ctx, char c);
inline void handleStateExportSourceEnd(ParserContext& ctx, char c);
inline void handleStateExportDefault(ParserContext& ctx, char c);
inline void handleStateExportAll(ParserContext& ctx, char c);

// Import keyword detection - starting from 'i'
inline void handleStateNoneI(ParserContext& ctx, char c) {
    if (c == 'm') {
        // "import" - continue with import parsing
        ctx.state = STATE::NONE_IM;
    } else if (c == 'n') {
        // "interface" - continue with interface parsing
        ctx.state = STATE::NONE_IN;
    } else {
        throw std::runtime_error("Unexpected character after 'i': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateNoneIM(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::NONE_IMP;
    } else {
        throw std::runtime_error("Expected 'p' after 'im': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateNoneIMP(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_IMPO;
    } else {
        throw std::runtime_error("Expected 'o' after 'imp': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateNoneIMPO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_IMPOR;
    } else {
        throw std::runtime_error("Expected 'r' after 'impo': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateNoneIMPOR(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_IMPORT;
    } else {
        throw std::runtime_error("Expected 't' after 'impor': " + std::string(1, c));
    }
}

// Import keyword completion
inline void handleStateNoneIMPORT(ParserContext& ctx, char c) {
    if (c == ' ') {
        auto* importDecl = new ImportDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(importDecl);
        ctx.currentNode = importDecl;
        ctx.state = STATE::IMPORT_SPECIFIERS_START;
    } else {
        throw std::runtime_error("Expected ' ' after 'import': " + std::string(1, c));
    }
}

// Import specifiers start
inline void handleStateImportSpecifiersStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Named imports: import { x, y } from 'module'
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
    } else if (c == '*') {
        // Namespace import: import * as name from 'module'
        auto* nsSpecifier = new ImportNamespaceSpecifier(ctx.currentNode);
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode)) {
            importDecl->setNamespaceSpecifier(nsSpecifier);
        }
        ctx.currentNode = nsSpecifier;
        ctx.state = STATE::IMPORT_SPECIFIER_AS;
    } else if (isalpha(c) || c == '_') {
        // Default import: import name from 'module' or import name, { x } from 'module'
        auto* defaultSpecifier = new ImportDefaultSpecifier(ctx.currentNode);
        defaultSpecifier->local = std::string(1, c);
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode)) {
            importDecl->setDefaultSpecifier(defaultSpecifier);
        }
        ctx.currentNode = defaultSpecifier;
        ctx.state = STATE::IMPORT_SPECIFIER_LOCAL_NAME;
    } else if (c == '"' || c == '\'') {
        // Side-effect import: import 'module'
        ctx.state = STATE::IMPORT_SOURCE_START;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{', '*', identifier, or string after 'import ': " + std::string(1, c));
    }
}

// Import specifier name (inside braces)
inline void handleStateImportSpecifierName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building specifier name
        return;
    } else if (c == ',') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ImportSpecifier(ctx.currentNode);
        specifier->imported = name;
        specifier->local = name; // Default local name same as imported
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode->parent)) {
            importDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::IMPORT_SPECIFIER_SEPARATOR;
    } else if (c == ' ') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ImportSpecifier(ctx.currentNode);
        specifier->imported = name;
        specifier->local = name; // Default local name same as imported
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode->parent)) {
            importDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::IMPORT_SPECIFIER_AS;
    } else if (c == '}') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ImportSpecifier(ctx.currentNode);
        specifier->imported = name;
        specifier->local = name; // Default local name same as imported
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode->parent)) {
            importDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
    } else if (c == 'a' && ctx.code.substr(ctx.index - 1, 3) == " as") {
        // "as" keyword for aliasing
        ctx.state = STATE::IMPORT_SPECIFIER_AS;
        ctx.index += 2; // Skip "as"
    } else {
        throw std::runtime_error("Unexpected character in import specifier name: " + std::string(1, c));
    }
}

// Import specifier "as" keyword
inline void handleStateImportSpecifierAs(ParserContext& ctx, char c) {
    if (c == 'a') {
        // Start of "as"
        return;
    } else if (c == 's') {
        // End of "as"
        ctx.state = STATE::IMPORT_SPECIFIER_LOCAL_NAME;
    } else {
        throw std::runtime_error("Expected 'as' keyword: " + std::string(1, c));
    }
}

// Import specifier local name
inline void handleStateImportSpecifierLocalName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building local name
        return;
    } else if (c == ',') {
        // End of local name
        std::string localName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ImportSpecifier*>(ctx.currentNode)) {
            specifier->local = localName;
        } else if (auto* nsSpecifier = dynamic_cast<ImportNamespaceSpecifier*>(ctx.currentNode)) {
            nsSpecifier->local = localName;
        } else if (auto* defaultSpecifier = dynamic_cast<ImportDefaultSpecifier*>(ctx.currentNode)) {
            defaultSpecifier->local = localName;
        }
        ctx.state = STATE::IMPORT_SPECIFIER_SEPARATOR;
    } else if (c == '}') {
        // End of local name
        std::string localName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ImportSpecifier*>(ctx.currentNode)) {
            specifier->local = localName;
        } else if (auto* nsSpecifier = dynamic_cast<ImportNamespaceSpecifier*>(ctx.currentNode)) {
            nsSpecifier->local = localName;
        } else if (auto* defaultSpecifier = dynamic_cast<ImportDefaultSpecifier*>(ctx.currentNode)) {
            defaultSpecifier->local = localName;
        }
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
    } else if (c == 'f' && ctx.code.substr(ctx.index - 1, 4) == "from") {
        // "from" keyword
        ctx.state = STATE::IMPORT_FROM;
        ctx.index += 3; // Skip "from"
    } else {
        throw std::runtime_error("Unexpected character in import specifier local name: " + std::string(1, c));
    }
}

// Import specifier separator
inline void handleStateImportSpecifierSeparator(ParserContext& ctx, char c) {
    if (isalpha(c) || c == '_') {
        // Next specifier name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected identifier after ',': " + std::string(1, c));
    }
}

// Import specifiers end
inline void handleStateImportSpecifiersEnd(ParserContext& ctx, char c) {
    if (c == 'f' && ctx.code.substr(ctx.index - 1, 4) == "from") {
        // "from" keyword
        ctx.state = STATE::IMPORT_FROM;
        ctx.index += 3; // Skip "from"
    } else if (c == ',') {
        // Additional specifiers after default import
        ctx.state = STATE::IMPORT_SPECIFIERS_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'from' keyword: " + std::string(1, c));
    }
}

// Import "from" keyword
inline void handleStateImportFrom(ParserContext& ctx, char c) {
    if (c == 'f') {
        // Start of "from"
        return;
    } else if (c == 'm') {
        // End of "from"
        ctx.state = STATE::IMPORT_SOURCE_START;
    } else {
        throw std::runtime_error("Expected 'from' keyword: " + std::string(1, c));
    }
}

// Import source start
inline void handleStateImportSourceStart(ParserContext& ctx, char c) {
    if (c == '"' || c == '\'') {
        ctx.quoteChar = c;
        ctx.state = STATE::IMPORT_SOURCE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected string literal for import source: " + std::string(1, c));
    }
}

// Import source
inline void handleStateImportSource(ParserContext& ctx, char c) {
    if (c == ctx.quoteChar) {
        // End of source string
        std::string source = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* importDecl = dynamic_cast<ImportDeclaration*>(ctx.currentNode)) {
            importDecl->source = source;
        }
        ctx.state = STATE::IMPORT_SOURCE_END;
    } else if (c == '\\') {
        // Escape sequence - for now just skip
        ctx.state = STATE::IMPORT_SOURCE;
    } else {
        // Continue building source
        return;
    }
}

// Import source end
inline void handleStateImportSourceEnd(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of import statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ';' after import source: " + std::string(1, c));
    }
}

// Export keyword detection - starting from 'e'
inline void handleStateNoneE(ParserContext& ctx, char c) {
    if (c == 'x') {
        // "export" - continue with export parsing
        ctx.state = STATE::NONE_EX;
    } else if (c == 'l') {
        // "else" - continue with else parsing
        ctx.state = STATE::NONE_EL;
    } else {
        throw std::runtime_error("Unexpected character after 'e': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateNoneEX(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::NONE_EXP;
    } else {
        throw std::runtime_error("Expected 'p' after 'ex': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateNoneEXP(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_EXPO;
    } else {
        throw std::runtime_error("Expected 'o' after 'exp': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateNoneEXPO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_EXPOR;
    } else {
        throw std::runtime_error("Expected 'r' after 'expo': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateNoneEXPOR(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_EXPORT;
    } else {
        throw std::runtime_error("Expected 't' after 'expor': " + std::string(1, c));
    }
}

// Export keyword completion
inline void handleStateNoneEXPORT(ParserContext& ctx, char c) {
    if (c == ' ') {
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
    } else {
        throw std::runtime_error("Expected ' ' after 'export': " + std::string(1, c));
    }
}

// Export specifiers start
inline void handleStateExportSpecifiersStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Named exports: export { x, y }
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
    } else if (c == '*') {
        // Re-export all: export * from 'module'
        auto* exportDecl = new ExportAllDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        ctx.state = STATE::EXPORT_ALL;
    } else if (c == 'd' && ctx.code.substr(ctx.index, 7) == "default") {
        // Export default
        ctx.state = STATE::EXPORT_DEFAULT;
        ctx.index += 6; // Skip "default"
    } else if (c == 'c' && ctx.code.substr(ctx.index, 5) == "const") {
        // Export const declaration
        ctx.state = STATE::NONE_C;
        // Re-process this character
        ctx.index--;
    } else if (c == 'l' && ctx.code.substr(ctx.index, 3) == "let") {
        // Export let declaration
        ctx.state = STATE::NONE_L;
        // Re-process this character
        ctx.index--;
    } else if (c == 'v' && ctx.code.substr(ctx.index, 3) == "var") {
        // Export var declaration
        ctx.state = STATE::NONE_V;
        // Re-process this character
        ctx.index--;
    } else if (c == 'f' && ctx.code.substr(ctx.index, 8) == "function") {
        // Export function declaration
        ctx.state = STATE::NONE_F;
        // Re-process this character
        ctx.index--;
    } else if (c == 'c' && ctx.code.substr(ctx.index, 5) == "class") {
        // Export class declaration
        ctx.state = STATE::NONE_CL;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Unexpected token after 'export ': " + std::string(1, c));
    }
}

// Export specifier name
inline void handleStateExportSpecifierName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building specifier name
        return;
    } else if (c == ',') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ExportSpecifier(ctx.currentNode);
        specifier->local = name;
        specifier->exported = name; // Default exported name same as local
        if (auto* exportDecl = dynamic_cast<ExportNamedDeclaration*>(ctx.currentNode)) {
            exportDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::EXPORT_SPECIFIER_SEPARATOR;
    } else if (c == ' ') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ExportSpecifier(ctx.currentNode);
        specifier->local = name;
        specifier->exported = name; // Default exported name same as local
        if (auto* exportDecl = dynamic_cast<ExportNamedDeclaration*>(ctx.currentNode)) {
            exportDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::EXPORT_SPECIFIER_AS;
    } else if (c == '}') {
        // End of specifier name, create specifier
        std::string name = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* specifier = new ExportSpecifier(ctx.currentNode);
        specifier->local = name;
        specifier->exported = name; // Default exported name same as local
        if (auto* exportDecl = dynamic_cast<ExportNamedDeclaration*>(ctx.currentNode)) {
            exportDecl->addSpecifier(specifier);
        }
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
    } else if (c == 'a' && ctx.code.substr(ctx.index - 1, 3) == " as") {
        // "as" keyword for aliasing
        ctx.state = STATE::EXPORT_SPECIFIER_AS;
        ctx.index += 2; // Skip "as"
    } else {
        throw std::runtime_error("Unexpected character in export specifier name: " + std::string(1, c));
    }
}

// Export specifier "as" keyword
inline void handleStateExportSpecifierAs(ParserContext& ctx, char c) {
    if (c == 'a') {
        // Start of "as"
        return;
    } else if (c == 's') {
        // End of "as"
        ctx.state = STATE::EXPORT_SPECIFIER_EXPORTED_NAME;
    } else {
        throw std::runtime_error("Expected 'as' keyword: " + std::string(1, c));
    }
}

// Export specifier exported name
inline void handleStateExportSpecifierExportedName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building exported name
        return;
    } else if (c == ',') {
        // End of exported name
        std::string exportedName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ExportSpecifier*>(ctx.currentNode->children.back())) {
            specifier->exported = exportedName;
        }
        ctx.state = STATE::EXPORT_SPECIFIER_SEPARATOR;
    } else if (c == '}') {
        // End of exported name
        std::string exportedName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ExportSpecifier*>(ctx.currentNode->children.back())) {
            specifier->exported = exportedName;
        }
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
    } else {
        throw std::runtime_error("Unexpected character in export specifier exported name: " + std::string(1, c));
    }
}

// Export specifier separator
inline void handleStateExportSpecifierSeparator(ParserContext& ctx, char c) {
    if (isalpha(c) || c == '_') {
        // Next specifier name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected identifier after ',': " + std::string(1, c));
    }
}

// Export specifiers end
inline void handleStateExportSpecifiersEnd(ParserContext& ctx, char c) {
