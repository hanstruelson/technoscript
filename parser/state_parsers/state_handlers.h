#pragma once

// Main state handlers file - includes all separated state handler files
// Organized by logical groupings and dependency order:
// - common_states.h: Root/entry point states (no dependencies)
// - variable_states.h: Variable keyword parsing (depends on common_states)
// - identifier_states.h: Identifier parsing (depends on variable_states)
// - type_annotation_states.h: Type annotation and equals parsing (depends on identifier_states)
// - expression_states.h: Expression parsing (depends on common_states)

#include "common_states.h"
#include "variable_states.h"
#include "identifier_states.h"
#include "type_annotation_states.h"
#include "generic_states.h"
#include "advanced_generic_states.h"
#include "expression_states.h"
#include "function_states.h"
#include "literal_states.h"
#include "control_flow_states.h"
#include "operator_states.h"
#include "class_states.h"
#include "interface_states.h"
#include "type_alias_states.h"
#include "module_states.h"
#include "destructuring_states.h"
#include "async_states.h"

// Enum state handlers
inline void handleStateNoneEnumE(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::NONE_ENUM_EN;
    } else {
        // Not "enum", backtrack
        ctx.index -= 2;  // Backtrack to 'e' and 'n'
        ctx.state = STATE::NONE;
    }
}

inline void handleStateNoneEnumEN(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::NONE_ENUM_ENU;
    } else {
        // Not "enum", backtrack
        ctx.index -= 3;  // Backtrack to 'e', 'n', 'u'
        ctx.state = STATE::NONE;
    }
}

inline void handleStateNoneEnumENU(ParserContext& ctx, char c) {
    if (c == ' ') {
        ctx.stringStart = 0;  // Reset for name parsing
        ctx.state = STATE::ENUM_DECLARATION_NAME;
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // Enum name starts immediately after "enum"
        ctx.stringStart = ctx.index - 4;  // Include "enum"
        ctx.state = STATE::ENUM_DECLARATION_NAME;
        ctx.index--;  // Re-process this character
    } else {
        // Not "enum", backtrack
        ctx.index -= 4;  // Backtrack to 'e', 'n', 'u', 'm'
        ctx.state = STATE::NONE;
    }
}

inline void handleStateNoneEnumENUM(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Check if next non-space character is 'c' for const enum
        std::size_t tempIndex = ctx.index;
        while (tempIndex < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[tempIndex]))) {
            tempIndex++;
        }
        if (tempIndex < ctx.code.length() && ctx.code[tempIndex] == 'c') {
            // This is a const enum
            ctx.state = STATE::NONE_CONS;  // Let const handler take over
            return;
        }
        ctx.stringStart = 0;  // Reset for name parsing
        ctx.state = STATE::ENUM_DECLARATION_NAME;
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // Enum name starts immediately
        ctx.stringStart = ctx.index - 4;  // Include "enum"
        ctx.state = STATE::ENUM_DECLARATION_NAME;
        ctx.index--;  // Re-process this character
    } else {
        throw std::runtime_error("Unexpected character after 'enum': " + std::string(1, c));
    }
}

inline void handleStateEnumDeclarationName(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // If this is the first character of the name, set stringStart
        if (ctx.stringStart == 0) {
            ctx.stringStart = ctx.index - 1;
        }
        return;  // Continue collecting enum name
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;  // Skip whitespace
    }

    if (c == '{') {
        // End of enum name, start enum body
        std::string enumName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        // Trim whitespace
        enumName.erase(enumName.find_last_not_of(" \t\n\r\f\v") + 1);

        // Check if enum declaration node is already created (for const enums)
        EnumDeclarationNode* enumDecl;
        if (ctx.currentNode->nodeType == ASTNodeType::ENUM_DECLARATION) {
            enumDecl = static_cast<EnumDeclarationNode*>(ctx.currentNode);
        } else {
            // Create enum declaration node
            enumDecl = new EnumDeclarationNode(ctx.currentNode);
            ctx.currentNode->addChild(enumDecl);
            ctx.currentNode = enumDecl;
        }
        enumDecl->name = enumName;

        ctx.state = STATE::ENUM_BODY_START;
    } else {
        throw std::runtime_error("Expected '{' after enum name, got: " + std::string(1, c));
    }
}

inline void handleStateEnumBodyStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '}') {
        // Empty enum
        ctx.state = STATE::ENUM_BODY;
        ctx.index--;  // Re-process the closing brace
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // Start of enum member name
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::ENUM_MEMBER_NAME;
        return;
    }

    throw std::runtime_error("Unexpected character in enum body: " + std::string(1, c));
}

inline void handleStateEnumMemberName(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;  // Continue collecting member name
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;  // Skip whitespace
    }

    if (c == '=') {
        // Member has initializer
        std::string memberName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        memberName.erase(memberName.find_last_not_of(" \t\n\r\f\v") + 1);

        // Create enum member
        auto* enumMember = new EnumMemberNode(ctx.currentNode);
        enumMember->name = memberName;
        ctx.currentNode->addChild(enumMember);
        // Add to enum declaration members
        if (auto* enumDecl = dynamic_cast<EnumDeclarationNode*>(ctx.currentNode)) {
            enumDecl->members.push_back(enumMember);
        }
        ctx.currentNode = enumMember;

        ctx.state = STATE::ENUM_MEMBER_INITIALIZER;
    } else if (c == ',' || c == '}') {
        // Member without initializer
        std::string memberName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        memberName.erase(memberName.find_last_not_of(" \t\n\r\f\v") + 1);

        // Create enum member
        auto* enumMember = new EnumMemberNode(ctx.currentNode);
        enumMember->name = memberName;
        ctx.currentNode->addChild(enumMember);
        // Add to enum declaration members
        if (auto* enumDecl = dynamic_cast<EnumDeclarationNode*>(ctx.currentNode)) {
            enumDecl->members.push_back(enumMember);
        }

        ctx.state = STATE::ENUM_MEMBER_SEPARATOR;
        ctx.index--;  // Re-process comma or closing brace
    } else {
        throw std::runtime_error("Unexpected character after enum member name: " + std::string(1, c));
    }
}

inline void handleStateEnumMemberInitializer(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    // For now, we'll handle simple numeric and string literals
    if (std::isdigit(static_cast<unsigned char>(c)) || c == '"' || c == '\'' || c == '-' || c == '+') {
        // Start of literal value
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        ctx.index--;  // Re-process this character
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // Reference to another enum member or identifier
        ctx.state = STATE::EXPRESSION_IDENTIFIER;
        ctx.index--;  // Re-process this character
    } else {
        throw std::runtime_error("Unexpected character in enum member initializer: " + std::string(1, c));
    }
}

inline void handleStateEnumMemberSeparator(ParserContext& ctx, char c) {
    if (c == ',') {
        // More members coming
        ctx.state = STATE::ENUM_BODY;
    } else if (c == '}') {
        // End of enum
        ctx.currentNode = ctx.currentNode->parent;  // Go back to enum declaration
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;  // Skip whitespace
    } else {
        throw std::runtime_error("Expected ',' or '}' after enum member, got: " + std::string(1, c));
    }
}

inline void handleStateEnumBody(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '}') {
        // End of enum
        ctx.currentNode = ctx.currentNode->parent;  // Go back to enum declaration
        ctx.state = STATE::NONE;
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        // Next enum member
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::ENUM_MEMBER_NAME;
    } else {
        throw std::runtime_error("Unexpected character in enum body: " + std::string(1, c));
    }
}

// Interface extends keyword handlers
inline void handleStateInterfaceExtendsE(ParserContext& ctx, char c) {
    if (c == 'x') {
        ctx.state = STATE::INTERFACE_EXTENDS_EX;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceExtendsEx(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::INTERFACE_EXTENDS_EXT;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 2; // Re-process "ex"
    }
}

inline void handleStateInterfaceExtendsExt(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::INTERFACE_EXTENDS_EXTE;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 3; // Re-process "ext"
    }
}

inline void handleStateInterfaceExtendsExte(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::INTERFACE_EXTENDS_EXTEN;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 4; // Re-process "exte"
    }
}

inline void handleStateInterfaceExtendsExten(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::INTERFACE_EXTENDS_EXTEND;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 5; // Re-process "exten"
    }
}

inline void handleStateInterfaceExtendsExtend(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::INTERFACE_EXTENDS_EXTENDS;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 6; // Re-process "extend"
    }
}

inline void handleStateInterfaceExtendsExtends(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "extends"
        while (ctx.index < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[ctx.index]))) {
            ctx.index++;
        }
        // Now parse the extended interface name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::INTERFACE_EXTENDS_NAME;
    } else {
        // Not valid after "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index -= 7; // Re-process "extends"
    }
}

// Interface readonly keyword handlers
inline void handleStateInterfaceReadonlyR(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::INTERFACE_READONLY_RE;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 2; // Include 'r' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyRe(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::INTERFACE_READONLY_REA;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 3; // Include 'r','e' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyRea(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::INTERFACE_READONLY_READ;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 4; // Include 'r','e','a' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyRead(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::INTERFACE_READONLY_READO;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 5; // Include 'r','e','a','d' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyReado(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::INTERFACE_READONLY_READON;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 6; // Include 'r','e','a','d','o' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyReadon(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::INTERFACE_READONLY_READONL;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 7; // Include 'r','e','a','d','o','n' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyReadonl(ParserContext& ctx, char c) {
    if (c == 'y') {
        ctx.state = STATE::INTERFACE_READONLY_READONLY;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 8; // Include 'r','e','a','d','o','n','l' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceReadonlyReadonly(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "readonly"
        return;
    } else if (isIdentifierStart(c)) {
        // Start of property name after readonly
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        // Mark that this property is readonly (handled in property creation)
    } else {
        // Not valid after "readonly", treat as property name
        ctx.stringStart = ctx.index - 9; // Include entire "readonly" + current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

// Class extends keyword handlers
inline void handleStateClassExtendsE(ParserContext& ctx, char c) {
    if (c == 'x') {
        ctx.state = STATE::CLASS_EXTENDS_EX;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassExtendsEx(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::CLASS_EXTENDS_EXT;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 2; // Re-process "ex"
    }
}

inline void handleStateClassExtendsExt(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_EXTENDS_EXTE;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 3; // Re-process "ext"
    }
}

inline void handleStateClassExtendsExte(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::CLASS_EXTENDS_EXTEN;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 4; // Re-process "exte"
    }
}

inline void handleStateClassExtendsExten(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::CLASS_EXTENDS_EXTEND;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 5; // Re-process "exten"
    }
}

inline void handleStateClassExtendsExtend(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::CLASS_EXTENDS_EXTENDS;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 6; // Re-process "extend"
    }
}

inline void handleStateClassExtendsExtends(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "extends"
        while (ctx.index < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[ctx.index]))) {
            ctx.index++;
        }
        // Now parse the extended class name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_EXTENDS_NAME;
    } else {
        // Not valid after "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 7; // Re-process "extends"
    }
}

// Class implements keyword handlers
inline void handleStateClassImplementsI(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IM;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsIm(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMP;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 2; // Re-process "im"
    }
}

inline void handleStateClassImplementsImp(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPL;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 3; // Re-process "imp"
    }
}

inline void handleStateClassImplementsImpl(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPLE;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 4; // Re-process "impl"
    }
}

inline void handleStateClassImplementsImple(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPLEM;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 5; // Re-process "imple"
    }
}

inline void handleStateClassImplementsImplem(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPLEME;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 6; // Re-process "implem"
    }
}

inline void handleStateClassImplementsImpleme(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPLEMEN;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 7; // Re-process "impleme"
    }
}

inline void handleStateClassImplementsImplemen(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::CLASS_IMPLEMENTS_IMPLEMENTS;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 8; // Re-process "implemen"
    }
}

inline void handleStateClassImplementsImplements(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "implements"
        while (ctx.index < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[ctx.index]))) {
            ctx.index++;
        }
        // Now parse the implemented interface name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_IMPLEMENTS_NAME;
    } else {
        // Not valid after "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index -= 10; // Re-process "implements"
    }
}

// Module import/export keyword handlers
inline void handleStateImportAsA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::IMPORT_AS_AS;
    } else {
        // Not "as", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportAsAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "as"
        ctx.state = STATE::IMPORT_SPECIFIER_LOCAL_NAME;
    } else {
        // Not valid after "as", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index -= 2; // Re-process "as"
    }
}

inline void handleStateImportFromF(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::IMPORT_FROM_FR;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportFromFr(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::IMPORT_FROM_FRO;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index -= 2; // Re-process "fr"
    }
}

inline void handleStateImportFromFro(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::IMPORT_FROM_FROM;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index -= 3; // Re-process "fro"
    }
}

inline void handleStateImportFromFrom(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "from"
        ctx.state = STATE::IMPORT_SOURCE_START;
    } else {
        // Not valid after "from", backtrack
        ctx.state = STATE::IMPORT_SPECIFIERS_END;
        ctx.index -= 4; // Re-process "from"
    }
}

inline void handleStateExportAsA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_AS_AS;
    } else {
        // Not "as", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportAsAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "as"
        ctx.state = STATE::EXPORT_SPECIFIER_EXPORTED_NAME;
    } else {
        // Not valid after "as", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index -= 2; // Re-process "as"
    }
}

inline void handleStateExportFromF(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::EXPORT_FROM_FR;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFromFr(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::EXPORT_FROM_FRO;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index -= 2; // Re-process "fr"
    }
}

inline void handleStateExportFromFro(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::EXPORT_FROM_FROM;
    } else {
        // Not "from", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index -= 3; // Re-process "fro"
    }
}

inline void handleStateExportFromFrom(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "from"
        ctx.state = STATE::EXPORT_SOURCE_START;
    } else {
        // Not valid after "from", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_END;
        ctx.index -= 4; // Re-process "from"
    }
}

inline void handleStateExportDefaultD(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::EXPORT_DEFAULT_DE;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportDefaultDe(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::EXPORT_DEFAULT_DEF;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "de"
    }
}

inline void handleStateExportDefaultDef(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFA;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "def"
    }
}

inline void handleStateExportDefaultDefa(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAU;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 4; // Re-process "defa"
    }
}

inline void handleStateExportDefaultDefau(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAUL;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 5; // Re-process "defau"
    }
}

inline void handleStateExportDefaultDefaul(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAULT;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 6; // Re-process "defaul"
    }
}

inline void handleStateExportDefaultDefault(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "default"
        ctx.state = STATE::EXPORT_DEFAULT;
    } else {
        // Not valid after "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 7; // Re-process "default"
    }
}

inline void handleStateExportConstC(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::EXPORT_CONST_CO;
    } else {
        // Not "const", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportConstCo(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::EXPORT_CONST_CON;
    } else {
        // Not "const", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "co"
    }
}

inline void handleStateExportConstCon(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_CONST_CONS;
    } else {
        // Not "const", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "con"
    }
}

inline void handleStateExportConstCons(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_CONST_CONST;
    } else {
        // Not "const", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 4; // Re-process "cons"
    }
}

inline void handleStateExportConstConst(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "const"
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create const variable inside export
        auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::CONST);
        exportDecl->children.push_back(varDecl);
        ctx.currentNode = varDecl;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        // Not valid after "const", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 5; // Re-process "const"
    }
}

inline void handleStateExportClassC(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::EXPORT_CLASS_CL;
    } else {
        // Not "class", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportClassCl(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::EXPORT_CLASS_CLA;
    } else {
        // Not "class", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "cl"
    }
}

inline void handleStateExportClassCla(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_CLASS_CLAS;
    } else {
        // Not "class", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "cla"
    }
}

inline void handleStateExportClassClas(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_CLASS_CLASS;
    } else {
        // Not "class", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 4; // Re-process "clas"
    }
}

inline void handleStateExportClassClass(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "class"
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create class inside export
        auto* classDecl = new ClassDeclarationNode(exportDecl);
        exportDecl->children.push_back(classDecl);
        ctx.currentNode = classDecl;
        ctx.state = STATE::CLASS_DECLARATION_NAME;
    } else {
        // Not valid after "class", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 5; // Re-process "class"
    }
}

inline void handleStateExportFunctionF(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::EXPORT_FUNCTION_FU;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFunctionFu(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::EXPORT_FUNCTION_FUN;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "fu"
    }
}

inline void handleStateExportFunctionFun(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::EXPORT_FUNCTION_FUNC;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "fun"
    }
}

inline void handleStateExportFunctionFunc(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_FUNCTION_FUNCT;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 4; // Re-process "func"
    }
}

inline void handleStateExportFunctionFunct(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::EXPORT_FUNCTION_FUNCTI;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 5; // Re-process "funct"
    }
}

inline void handleStateExportFunctionFuncti(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::EXPORT_FUNCTION_FUNCTIO;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 6; // Re-process "functi"
    }
}

inline void handleStateExportFunctionFunctio(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::EXPORT_FUNCTION_FUNCTION;
    } else {
        // Not "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 7; // Re-process "functio"
    }
}

inline void handleStateExportFunctionFunction(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "function"
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create function inside export
        auto* funcDecl = new FunctionDeclarationNode(exportDecl);
        exportDecl->children.push_back(funcDecl);
        ctx.currentNode = funcDecl;
        ctx.state = STATE::FUNCTION_DECLARATION_NAME;
    } else {
        // Not valid after "function", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 8; // Re-process "function"
    }
}

inline void handleStateExportLetL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::EXPORT_LET_LE;
    } else {
        // Not "let", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportLetLe(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_LET_LET;
    } else {
        // Not "let", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "le"
    }
}

inline void handleStateExportLetLet(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "let"
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create let variable inside export
        auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::LET);
        exportDecl->children.push_back(varDecl);
        ctx.currentNode = varDecl;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        // Not valid after "let", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "let"
    }
}

inline void handleStateExportVarV(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::EXPORT_VAR_VA;
    } else {
        // Not "var", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportVarVa(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::EXPORT_VAR_VAR;
    } else {
        // Not "var", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 2; // Re-process "va"
    }
}

inline void handleStateExportVarVar(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "var"
        auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
        ctx.currentNode->children.push_back(exportDecl);
        ctx.currentNode = exportDecl;
        // Create var variable inside export
        auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::VAR);
        exportDecl->children.push_back(varDecl);
        ctx.currentNode = varDecl;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        // Not valid after "var", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index -= 3; // Re-process "var"
    }
}
