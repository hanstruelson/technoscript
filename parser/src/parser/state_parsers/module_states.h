#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Forward declarations for module state handlers
inline void handleStateBlockIM(ParserContext& ctx, char c);
inline void handleStateBlockIMP(ParserContext& ctx, char c);
inline void handleStateBlockIMPO(ParserContext& ctx, char c);
inline void handleStateBlockIMPOR(ParserContext& ctx, char c);
inline void handleStateBlockIMPORT(ParserContext& ctx, char c);
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
inline void handleStateBlockEX(ParserContext& ctx, char c);
inline void handleStateBlockEXP(ParserContext& ctx, char c);
inline void handleStateBlockEXPO(ParserContext& ctx, char c);
inline void handleStateBlockEXPOR(ParserContext& ctx, char c);
inline void handleStateBlockEXPORT(ParserContext& ctx, char c);
inline void handleStateExportSpecifiersStart(ParserContext& ctx, char c);
inline void handleStateExportSpecifierName(ParserContext& ctx, char c);
inline void handleStateExportSpecifierAs(ParserContext& ctx, char c);
inline void handleStateExportSpecifierExportedName(ParserContext& ctx, char c);
inline void handleStateExportSpecifierSeparator(ParserContext& ctx, char c);
inline void handleStateExportSpecifiersEnd(ParserContext& ctx, char c);
inline void handleStateExportFromF(ParserContext& ctx, char c);
inline void handleStateExportFromFr(ParserContext& ctx, char c);
inline void handleStateExportFromFro(ParserContext& ctx, char c);
inline void handleStateExportSourceStart(ParserContext& ctx, char c);
inline void handleStateExportSource(ParserContext& ctx, char c);
inline void handleStateExportSourceEnd(ParserContext& ctx, char c);
inline void handleStateExportDefault(ParserContext& ctx, char c);
inline void handleStateExportAll(ParserContext& ctx, char c);
inline void handleStateExportDeclaration(ParserContext& ctx, char c);

// Note: handleStateBlockI is defined in control_flow_states.h
// Note: handleStateBlockE is defined in common_states.h

// Import keyword continuation
inline void handleStateBlockIM(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::BLOCK_IMP;
    } else {
        throw std::runtime_error("Expected 'p' after 'im': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateBlockIMP(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::BLOCK_IMPO;
    } else {
        throw std::runtime_error("Expected 'o' after 'imp': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateBlockIMPO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::BLOCK_IMPOR;
    } else {
        throw std::runtime_error("Expected 'r' after 'impo': " + std::string(1, c));
    }
}

// Import keyword continuation
inline void handleStateBlockIMPOR(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::BLOCK_IMPORT;
    } else {
        throw std::runtime_error("Expected 't' after 'impor': " + std::string(1, c));
    }
}

// Import keyword completion
inline void handleStateBlockIMPORT(ParserContext& ctx, char c) {
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
        ctx.stringStart = 0; // Not set yet
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
        ctx.stringStart = ctx.index; // Start of identifier
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
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    }

    if (ctx.stringStart == 0) {
        ctx.stringStart = ctx.index - 1;
    }

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
    } else if (c == 'a') {
        // "as" keyword for aliasing
        ctx.state = STATE::POST_IMPORT_SPECIFIER_A;
    } else {
        throw std::runtime_error("Unexpected character in import specifier name: " + std::string(1, c));
    }
}

// Import specifier after name (handles "as" or other endings)
inline void handleStateImportSpecifierAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (c == 'a') {
        // "as" keyword
        ctx.state = STATE::POST_IMPORT_SPECIFIER_A;
    } else if (c == ',') {
        // Next specifier
        ctx.state = STATE::IMPORT_SPECIFIER_SEPARATOR;
    } else if (c == '}') {
        // End of specifiers
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
    } else if (c == 'f') {
        // "from" keyword
        ctx.state = STATE::IMPORT_FROM_F;
    } else {
        throw std::runtime_error("Expected 'as', ',', '}', or 'from' after specifier name: " + std::string(1, c));
    }
}

// Import specifier local name
inline void handleStateImportSpecifierLocalName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (isalnum(c) || c == '_') {
        // Continue building local name
        return;
    } else if (c == ',') {
        // End of local name - for default imports, this means additional specifiers follow
        std::string localName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ImportSpecifier*>(ctx.currentNode)) {
            specifier->local = localName;
            ctx.state = STATE::IMPORT_SPECIFIER_SEPARATOR;
        } else if (auto* nsSpecifier = dynamic_cast<ImportNamespaceSpecifier*>(ctx.currentNode)) {
            nsSpecifier->local = localName;
            ctx.state = STATE::IMPORT_SPECIFIERS_END;
        } else if (auto* defaultSpecifier = dynamic_cast<ImportDefaultSpecifier*>(ctx.currentNode)) {
            defaultSpecifier->local = localName;
            // For default imports, comma means additional specifiers
            ctx.currentNode = ctx.currentNode->parent; // Go back to ImportDeclaration
            ctx.state = STATE::IMPORT_SPECIFIERS_START;
        }
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
    } else if (c == 'f') {
        // "from" keyword start
        std::string localName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ImportSpecifier*>(ctx.currentNode)) {
            specifier->local = localName;
        } else if (auto* nsSpecifier = dynamic_cast<ImportNamespaceSpecifier*>(ctx.currentNode)) {
            nsSpecifier->local = localName;
        } else if (auto* defaultSpecifier = dynamic_cast<ImportDefaultSpecifier*>(ctx.currentNode)) {
            defaultSpecifier->local = localName;
        }
        ctx.state = STATE::IMPORT_FROM_F;
    } else if (c == '"' || c == '\'') {
        // Start of source string (no "from" keyword for side-effect imports)
        std::string localName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ImportSpecifier*>(ctx.currentNode)) {
            specifier->local = localName;
        } else if (auto* nsSpecifier = dynamic_cast<ImportNamespaceSpecifier*>(ctx.currentNode)) {
            nsSpecifier->local = localName;
        } else if (auto* defaultSpecifier = dynamic_cast<ImportDefaultSpecifier*>(ctx.currentNode)) {
            defaultSpecifier->local = localName;
        }
        ctx.state = STATE::IMPORT_SOURCE_START;
        ctx.index--; // Re-process this character
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
    if (c == 'f') {
        // "from" keyword
        ctx.state = STATE::IMPORT_FROM_F;
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
    } else if (c == 'r') {
        // Second character of "from"
        return;
    } else if (c == 'o') {
        // Third character of "from"
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
        ctx.stringStart = ctx.index; // Start of string content
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
        std::string source = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart - 1);
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
        ctx.state = STATE::BLOCK;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ';' after import source: " + std::string(1, c));
    }
}

// Note: handleStateBlockE is defined in common_states.h

// Export keyword continuation
inline void handleStateBlockEX(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::BLOCK_EXP;
    } else {
        throw std::runtime_error("Expected 'p' after 'ex': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateBlockEXP(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::BLOCK_EXPO;
    } else {
        throw std::runtime_error("Expected 'o' after 'exp': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateBlockEXPO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::BLOCK_EXPOR;
    } else {
        throw std::runtime_error("Expected 'r' after 'expo': " + std::string(1, c));
    }
}

// Export keyword continuation
inline void handleStateBlockEXPOR(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::BLOCK_EXPORT;
    } else {
        throw std::runtime_error("Expected 't' after 'expor': " + std::string(1, c));
    }
}

// Export keyword completion
inline void handleStateBlockEXPORT(ParserContext& ctx, char c) {
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
        ctx.stringStart = 0; // Will be set when first identifier character is encountered
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
    } else if (c == '*') {
        // Re-export all: export * from 'module'
        auto* exportDecl = new ExportAllDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        ctx.state = STATE::EXPORT_ALL;
    } else if (c == 'd') {
        // Export default
        ctx.state = STATE::EXPORT_DEFAULT_D;
    } else if (isIdentifierStart(c)) {
        // Identifier to export (could be keyword like const/let/var/function or regular identifier)
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPORT_IDENTIFIER;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Unexpected token after 'export ': " + std::string(1, c));
    }
}

// Export specifier name
inline void handleStateExportSpecifierName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    }

    if (ctx.stringStart == 0) {
        ctx.stringStart = ctx.index - 1;
    }

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
    } else if (c == 'a') {
        // "as" keyword for aliasing
        ctx.state = STATE::EXPORT_AS_A;
    } else {
        throw std::runtime_error("Unexpected character in export specifier name: " + std::string(1, c));
    }
}

// Export specifier after name (handles "as" or other endings)
inline void handleStateExportSpecifierAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (c == 'a') {
        // "as" keyword
        ctx.state = STATE::EXPORT_AS_A;
    } else if (c == ',') {
        // Next specifier
        ctx.state = STATE::EXPORT_SPECIFIER_SEPARATOR;
    } else if (c == '}') {
        // End of specifiers
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
    } else {
        throw std::runtime_error("Expected 'as', ',', '}' after specifier name: " + std::string(1, c));
    }
}

inline void handleStateExportA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_AS;
    } else {
        throw std::runtime_error("Expected 's' after 'a' in 'as': " + std::string(1, c));
    }
}

inline void handleStateExportAs(ParserContext& ctx, char c) {
    if (c == ' ') {
        // After "as", expect identifier for exported name
        ctx.state = STATE::EXPORT_SPECIFIER_EXPORTED_NAME;
        ctx.stringStart = ctx.index; // Start of exported name
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected space after 'as': " + std::string(1, c));
    }
}

// Export specifier exported name
inline void handleStateExportSpecifierExportedName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    }

    if (c == ',') {
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
    } else if (c == 'f') {
        // "from" keyword
        std::string exportedName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* specifier = dynamic_cast<ExportSpecifier*>(ctx.currentNode->children.back())) {
            specifier->exported = exportedName;
        }
        ctx.state = STATE::EXPORT_FROM_F;
    } else if (isalnum(c) || c == '_') {
        // Continue building exported name
        return;
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
    } else if (c == '}') {
        // End of specifiers
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected identifier or '}' after ',': " + std::string(1, c));
    }
}

inline void handleStateExportDefault(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Export default declaration
        auto* exportDecl = new ExportDefaultDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // For now, just skip to the next state - would need expression parsing
        ctx.state = STATE::BLOCK;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected space after 'export default': " + std::string(1, c));
    }
}

inline void handleStateExportAll(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Continue parsing "export * from 'module'" or "export * as name from 'module'"
        return;
    } else if (c == 'f') {
        // "from" keyword
        ctx.state = STATE::EXPORT_FROM_F;
    } else if (c == 'a') {
        // "as" keyword for aliasing
        ctx.state = STATE::EXPORT_AS_A;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'from' or 'as' after 'export *': " + std::string(1, c));
    }
}

// Note: handleStateExportFromFr, handleStateExportFromFro are defined in state_handlers.h

inline void handleStateExportFrom(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::EXPORT_FROM_FR;
    } else {
        throw std::runtime_error("Expected 'r' after 'f' in 'from': " + std::string(1, c));
    }
}

// Export source start
inline void handleStateExportSourceStart(ParserContext& ctx, char c) {
    if (c == '"' || c == '\'') {
        ctx.quoteChar = c;
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPORT_SOURCE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected string literal for export source: " + std::string(1, c));
    }
}

// Export source
inline void handleStateExportSource(ParserContext& ctx, char c) {
    if (c == ctx.quoteChar) {
        // End of source string
        std::string source = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart - 1);
        if (auto* exportDecl = dynamic_cast<ExportNamedDeclaration*>(ctx.currentNode)) {
            exportDecl->source = source;
        } else if (auto* exportAllDecl = dynamic_cast<ExportAllDeclaration*>(ctx.currentNode)) {
            exportAllDecl->source = source;
        }
        ctx.state = STATE::EXPORT_SOURCE_END;
    } else if (c == '\\') {
        // Escape sequence - for now just skip
        ctx.state = STATE::EXPORT_SOURCE;
    } else {
        // Continue building source
        return;
    }
}

// Export source end
inline void handleStateExportSourceEnd(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of export statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::BLOCK;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ';' after export source: " + std::string(1, c));
    }
}

// Export specifiers end
inline void handleStateExportSpecifiersEnd(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of export statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::BLOCK;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ';' after export specifiers: " + std::string(1, c));
    }
}

// Export declaration (for let/var)
inline void handleStateExportDeclaration(ParserContext& ctx, char c) {
    if (c == 'l') {
        // Export let declaration
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create let variable inside export
        auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::LET);
        exportDecl->children.push_back(varDecl);
        ctx.currentNode = varDecl;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else if (c == 'v') {
        // Export var declaration
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create var variable inside export
        auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::VAR);
        exportDecl->children.push_back(varDecl);
        ctx.currentNode = varDecl;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        throw std::runtime_error("Expected 'let' or 'var' after 'export': " + std::string(1, c));
    }
}
