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
        // Not "enum", treat as identifier
        ctx.stringStart = ctx.index - 2;  // Include 'e' and 'n'
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;  // Re-process current char
    }
}

inline void handleStateNoneEnumEN(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::NONE_ENUM_ENU;
    } else {
        // Not "enum", treat as identifier
        ctx.stringStart = ctx.index - 3;  // Include 'e', 'n', 'u'
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;  // Re-process current char
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
        // Not "enum", treat as identifier
        ctx.stringStart = ctx.index - 4;  // Include 'e', 'n', 'u', 'm'
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;  // Re-process current char
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

// Interface after name keyword handlers
inline void handleStateInterfaceAfterNameE(ParserContext& ctx, char c) {
    if (c == 'x') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EX;
    } else {
        // Not "extends", reset to interface body
        ctx.state = STATE::INTERFACE_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameEx(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EXT;
    } else {
        // Not "extends", treat as property name
        ctx.stringStart = ctx.index - 3; // Include 'e','x' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameExt(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EXTE;
    } else {
        // Not "extends", treat as property name
        ctx.stringStart = ctx.index - 4; // Include 'e','x','t' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameExte(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EXTEN;
    } else {
        // Not "extends", treat as property name
        ctx.stringStart = ctx.index - 5; // Include 'e','x','t','e' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameExten(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EXTEND;
    } else {
        // Not "extends", treat as property name
        ctx.stringStart = ctx.index - 6; // Include 'e','x','t','e','n' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameExtend(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::INTERFACE_AFTER_NAME_EXTENDS;
    } else {
        // Not "extends", treat as property name
        ctx.stringStart = ctx.index - 7; // Include 'e','x','t','e','n','d' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceAfterNameExtends(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "extends"
        while (ctx.index < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[ctx.index]))) {
            ctx.index++;
        }
        // Now parse the extended interface name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::INTERFACE_AFTER_NAME_NAME;
    } else {
        // Not valid after "extends", treat as property name
        ctx.stringStart = ctx.index - 8; // Include entire "extends" + current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

// Interface member modifier keyword handlers
inline void handleStateInterfaceMemberR(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::INTERFACE_MEMBER_RE;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 2; // Include 'r' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberRe(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::INTERFACE_MEMBER_REA;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 3; // Include 'r','e' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberRea(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::INTERFACE_MEMBER_READ;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 4; // Include 'r','e','a' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberRead(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::INTERFACE_MEMBER_READO;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 5; // Include 'r','e','a','d' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberReado(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::INTERFACE_MEMBER_READON;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 6; // Include 'r','e','a','d','o' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberReadon(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::INTERFACE_MEMBER_READONL;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 7; // Include 'r','e','a','d','o','n' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberReadonl(ParserContext& ctx, char c) {
    if (c == 'y') {
        ctx.state = STATE::INTERFACE_MEMBER_READONLY;
    } else {
        // Not "readonly", treat as property name
        ctx.stringStart = ctx.index - 8; // Include 'r','e','a','d','o','n','l' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberReadonly(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "readonly"
        return;
    } else if (isIdentifierStart(c)) {
        // Start of property name after readonly - create the property node now
        auto* propNode = new InterfacePropertyNode(ctx.currentNode);
        propNode->isReadonly = true;
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addInterfaceProperty(propNode);
        }
        ctx.currentNode = propNode;
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
    } else {
        // Not valid after "readonly", treat as property name
        ctx.stringStart = ctx.index - 9; // Include entire "readonly" + current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

// Interface member "new" keyword handlers
inline void handleStateInterfaceMemberN(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::INTERFACE_MEMBER_NE;
    } else {
        // Not "new", treat as property name
        ctx.stringStart = ctx.index - 2; // Include 'n' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberNe(ParserContext& ctx, char c) {
    if (c == 'w') {
        ctx.state = STATE::INTERFACE_MEMBER_NEW;
    } else {
        // Not "new", treat as property name
        ctx.stringStart = ctx.index - 3; // Include 'n','e' and current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateInterfaceMemberNew(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "new"
        return;
    } else if (c == '(') {
        // Start of construct signature
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_START;
        ctx.index--; // Re-process this character
    } else {
        // Not valid after "new", treat as property name
        ctx.stringStart = ctx.index - 4; // Include entire "new" + current char
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    }
}

// Class after name keyword handlers
inline void handleStateClassAfterNameStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return; // Skip whitespace
    } else if (c == 'e') {
        ctx.state = STATE::CLASS_AFTER_NAME_E;
    } else if (c == 'i') {
        ctx.state = STATE::CLASS_INHERITANCE_I;
    } else if (c == '{') {
        // Start of class body
        ctx.state = STATE::CLASS_BODY;
    } else {
        // Not a keyword, unexpected character
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameName(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return; // Continue collecting name
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return; // Skip whitespace
    }

    if (c == '{') {
        // End of name, set extends class
        std::string name = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        name.erase(name.find_last_not_of(" \t\n\r\f\v") + 1);

        // Set class extends
        if (auto* classDecl = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classDecl->extendsClass = name;
        }

        ctx.state = STATE::CLASS_BODY;
    } else {
        throw std::runtime_error("Unexpected character after class extends name: " + std::string(1, c));
    }
}

inline void handleStateClassAfterNameE(ParserContext& ctx, char c) {
    if (c == 'x') {
        ctx.state = STATE::CLASS_AFTER_NAME_EX;
    } else {
        // Not "extends", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameEx(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::CLASS_AFTER_NAME_EXT;
    } else {
        // Not "extends", treat as identifier
        ctx.stringStart = ctx.index - 3; // Include 'e','x' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameExt(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_AFTER_NAME_EXTE;
    } else {
        // Not "extends", treat as identifier
        ctx.stringStart = ctx.index - 4; // Include 'e','x','t' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameExte(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::CLASS_AFTER_NAME_EXTEN;
    } else {
        // Not "extends", treat as identifier
        ctx.stringStart = ctx.index - 5; // Include 'e','x','t','e' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameExten(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::CLASS_AFTER_NAME_EXTEND;
    } else {
        // Not "extends", treat as identifier
        ctx.stringStart = ctx.index - 6; // Include 'e','x','t','e','n' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameExtend(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::CLASS_AFTER_NAME_EXTENDS;
    } else {
        // Not "extends", treat as identifier
        ctx.stringStart = ctx.index - 7; // Include 'e','x','t','e','n','d' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassAfterNameExtends(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "extends"
        while (ctx.index < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[ctx.index]))) {
            ctx.index++;
        }
        // Now parse the extended class name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_AFTER_NAME_NAME;
    } else {
        // Not valid after "extends", treat as identifier
        ctx.stringStart = ctx.index - 8; // Include entire "extends" + current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

// Class implements keyword handlers
inline void handleStateClassImplementsI(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::CLASS_INHERITANCE_IM;
    } else {
        // Not "implements", reset to class body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsIm(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::CLASS_INHERITANCE_IMP;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 3; // Include 'i','m' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImp(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPL;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 4; // Include 'i','m','p' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImpl(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPLE;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 5; // Include 'i','m','p','l' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImple(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPLEM;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 6; // Include 'i','m','p','l','e' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImplem(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPLEME;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 7; // Include 'i','m','p','l','e','m' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImpleme(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPLEMEN;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 8; // Include 'i','m','p','l','e','m','e' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateClassImplementsImplemen(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::CLASS_INHERITANCE_IMPLEMENTS;
    } else {
        // Not "implements", treat as identifier
        ctx.stringStart = ctx.index - 9; // Include 'i','m','p','l','e','m','e','n' and current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
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
        // Not valid after "implements", treat as identifier
        ctx.stringStart = ctx.index - 10; // Include entire "implements" + current char
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

// Module import/export keyword handlers
inline void handleStateImportAsA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::IMPORT_SPECIFIER_AFTER_AS;
    } else {
        // Not "as", treat as specifier name
        ctx.stringStart = ctx.index - 2; // Include 'a' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportAsAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "as"
        ctx.state = STATE::IMPORT_SPECIFIER_LOCAL_NAME;
    } else {
        // Not valid after "as", treat as specifier name
        ctx.stringStart = ctx.index - 3; // Include 'a','s' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportFromF(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::IMPORT_FROM_FR;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 2; // Include 'f' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportFromFr(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::IMPORT_FROM_FRO;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 3; // Include 'f','r' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportFromFro(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::IMPORT_FROM_FROM;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 4; // Include 'f','r','o' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateImportFromFrom(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "from"
        ctx.state = STATE::IMPORT_SOURCE_START;
    } else {
        // Not valid after "from", treat as specifier name
        ctx.stringStart = ctx.index - 5; // Include 'f','r','o','m' and current char
        ctx.state = STATE::IMPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportAsA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::EXPORT_SPECIFIER_AFTER_AS;
    } else {
        // Not "as", treat as specifier name
        ctx.stringStart = ctx.index - 2; // Include 'a' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportAsAs(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "as"
        ctx.state = STATE::EXPORT_SPECIFIER_EXPORTED_NAME;
    } else {
        // Not valid after "as", treat as specifier name
        ctx.stringStart = ctx.index - 3; // Include 'a','s' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFromF(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::EXPORT_FROM_FR;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 2; // Include 'f' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFromFr(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::EXPORT_FROM_FRO;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 3; // Include 'f','r' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFromFro(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::EXPORT_FROM_FROM;
    } else {
        // Not "from", treat as specifier name
        ctx.stringStart = ctx.index - 4; // Include 'f','r','o' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportFromFrom(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "from"
        ctx.state = STATE::EXPORT_SOURCE_START;
    } else {
        // Not valid after "from", treat as specifier name
        ctx.stringStart = ctx.index - 5; // Include 'f','r','o','m' and current char
        ctx.state = STATE::EXPORT_SPECIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExportDefaultD(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::EXPORT_DEFAULT_DE;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process this character
        ctx.stringStart = ctx.index - 1; // Include 'd' + current char
    }
}

inline void handleStateExportDefaultDe(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::EXPORT_DEFAULT_DEF;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 2; // Include 'd','e' + current char
    }
}

inline void handleStateExportDefaultDef(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFA;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 3; // Include 'd','e','f' + current char
    }
}

inline void handleStateExportDefaultDefa(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAU;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 4; // Include 'd','e','f','a' + current char
    }
}

inline void handleStateExportDefaultDefau(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAUL;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 5; // Include 'd','e','f','a','u' + current char
    }
}

inline void handleStateExportDefaultDefaul(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_DEFAULT_DEFAULT;
    } else {
        // Not "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 6; // Include 'd','e','f','a','u','l' + current char
    }
}

inline void handleStateExportDefaultDefault(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after "default"
        ctx.state = STATE::EXPORT_DEFAULT;
    } else {
        // Not valid after "default", backtrack
        ctx.state = STATE::EXPORT_SPECIFIERS_START;
        ctx.index--; // Re-process current char
        ctx.stringStart = ctx.index - 7; // Include 'd','e','f','a','u','l','t' + current char
    }
}





inline void handleStateExportV(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::EXPORT_VA;
    } else {
        // Not "var", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 1; // Include 'v'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportVa(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::EXPORT_VAR;
    } else {
        // Not "var", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 2; // Include 'v','a'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportVar(ParserContext& ctx, char c) {
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
        // Not valid after "var", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 3; // Include 'v','a','r'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportF(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::EXPORT_FU;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 1; // Include 'f'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFu(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::EXPORT_FUN;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 2; // Include 'f','u'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFun(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::EXPORT_FUNC;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 3; // Include 'f','u','n'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFunc(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::EXPORT_FUNCT;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 4; // Include 'f','u','n','c'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFunct(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::EXPORT_FUNCTI;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 5; // Include 'f','u','n','c','t'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFuncti(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::EXPORT_FUNCTIO;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 6; // Include 'f','u','n','c','t','i'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFunctio(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::EXPORT_FUNCTION;
    } else {
        // Not "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 7; // Include 'f','u','n','c','t','i','o'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportFunction(ParserContext& ctx, char c) {
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
        // Not valid after "function", backtrack and treat as identifier
        ctx.index--;
        ctx.stringStart = ctx.index - 8; // Include 'f','u','n','c','t','i','o','n'
        ctx.state = STATE::EXPORT_IDENTIFIER;
    }
}

inline void handleStateExportIdentifier(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // End of identifier, check if it's a keyword or regular identifier
        std::string identifier = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        identifier.erase(identifier.find_last_not_of(" \t\n\r\f\v") + 1);

        if (identifier == "const") {
            // Export const declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create const variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::CONST);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "let") {
            // Export let declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create let variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::LET);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "var") {
            // Export var declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create var variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::VAR);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "function") {
            // Export function declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create function inside export
            auto* funcDecl = new FunctionDeclarationNode(exportDecl);
            exportDecl->children.push_back(funcDecl);
            ctx.currentNode = funcDecl;
            ctx.state = STATE::FUNCTION_DECLARATION_NAME;
        } else {
            // Regular identifier to export
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            auto* specifier = new ExportSpecifier(exportDecl);
            specifier->local = identifier;
            specifier->exported = identifier;
            exportDecl->addSpecifier(specifier);
            ctx.state = STATE::EXPORT_SPECIFIERS_END;
        }
    } else if (c == ';') {
        // End of identifier, check if it's a keyword or regular identifier
        std::string identifier = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);

        if (identifier == "const") {
            // Export const declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create const variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::CONST);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "let") {
            // Export let declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create let variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::LET);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "var") {
            // Export var declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create var variable inside export
            auto* varDecl = new VariableDefinitionNode(exportDecl, VariableDefinitionType::VAR);
            exportDecl->children.push_back(varDecl);
            ctx.currentNode = varDecl;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        } else if (identifier == "function") {
            // Export function declaration
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            // Create function inside export
            auto* funcDecl = new FunctionDeclarationNode(exportDecl);
            exportDecl->children.push_back(funcDecl);
            ctx.currentNode = funcDecl;
            ctx.state = STATE::FUNCTION_DECLARATION_NAME;
        } else {
            // Regular identifier to export
            auto* exportDecl = new ExportNamedDeclaration(ctx.currentNode);
            ctx.currentNode->children.push_back(exportDecl);
            ctx.currentNode = exportDecl;
            auto* specifier = new ExportSpecifier(exportDecl);
            specifier->local = identifier;
            specifier->exported = identifier;
            exportDecl->addSpecifier(specifier);
            ctx.state = STATE::EXPORT_SPECIFIERS_END;
        }
        ctx.index--; // Re-process semicolon
    } else if (isalnum(c) || c == '_') {
        // Continue building identifier
        return;
    } else {
        throw std::runtime_error("Unexpected character in export identifier: " + std::string(1, c));
    }
}

// TechnoScript specific handlers

// Print statement handlers
inline void handleStateNoneP(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_PR;
    } else {
        // Not "print", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNonePR(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_PRI;
    } else {
        // Not "print", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNonePRI(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::NONE_PRIN;
    } else {
        // Not "print", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNonePRIN(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_PRINT;
    } else {
        // Not "print", treat as identifier
        ctx.stringStart = ctx.index - 4;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNonePRINT(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c)) || c == '(') {
        // Print statement
        auto* printStmt = new ASTNode(ctx.currentNode);
        printStmt->nodeType = AST_NODE;
        printStmt->value = "print";
        ctx.currentNode->addChild(printStmt);
        ctx.currentNode = printStmt;
        ctx.state = STATE::STATEMENT_PRINT;
        if (c == '(') {
            ctx.index--; // Re-process '('
        }
    } else {
        // Not "print", treat as identifier
        ctx.stringStart = ctx.index - 5;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateStatementPrint(ParserContext& ctx, char c) {
    if (c == '(') {
        // Parse arguments - create parenthesis for argument list
        auto* parenNode = new ParenthesisExpressionNode(ctx.currentNode);
        ctx.currentNode->addChild(parenNode);
        ctx.currentNode = parenNode;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        throw std::runtime_error("Expected '(' after print");
    }
}

// Go statement handlers
inline void handleStateNoneG(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_GO;
    } else {
        // Not "go", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneGO(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Go statement
        auto* goStmt = new ASTNode(ctx.currentNode);
        goStmt->nodeType = ASTNodeType::AST_NODE; // TODO: Add GO_STMT to AST
        goStmt->value = "go";
        ctx.currentNode->addChild(goStmt);
        ctx.currentNode = goStmt;
        ctx.state = STATE::STATEMENT_GO;
    } else {
        // Not "go", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateStatementGo(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Parse function call
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        ctx.index--; // Re-process this character
    }
}

// SetTimeout handlers
inline void handleStateNoneSE(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_SET;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSET(ParserContext& ctx, char c) {
    if (c == 'T') {
        ctx.state = STATE::NONE_SETT;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETT(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_SETTI;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETTI(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::NONE_SETTIM;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 4;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETTIM(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_SETTIMEO;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 5;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETTIMEO(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_SETTIMEOU;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 6;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETTIMEOU(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::NONE_SETTIMEOUT;
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 7;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSETTIMEOUT(ParserContext& ctx, char c) {
    if (c == '(') {
        // SetTimeout statement
        auto* setTimeoutStmt = new ASTNode(ctx.currentNode);
        setTimeoutStmt->nodeType = ASTNodeType::AST_NODE; // TODO: Add SETTIMEOUT_STMT to AST
        setTimeoutStmt->value = "setTimeout";
        ctx.currentNode->addChild(setTimeoutStmt);
        ctx.currentNode = setTimeoutStmt;
        ctx.state = STATE::STATEMENT_SETTIMEOUT;
        ctx.index--; // Re-process '('
    } else {
        // Not "setTimeout", treat as identifier
        ctx.stringStart = ctx.index - 8;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateStatementSetTimeout(ParserContext& ctx, char c) {
    if (c == '(') {
        // Parse arguments
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        throw std::runtime_error("Expected '(' after setTimeout");
    }
}

// Sleep handlers
inline void handleStateNoneSL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_SLE;
    } else {
        // Not "sleep", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSLE(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_SLEE;
    } else {
        // Not "sleep", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSLEE(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::NONE_SLEEP;
    } else {
        // Not "sleep", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneSLEEP(ParserContext& ctx, char c) {
    if (c == '(') {
        // Sleep statement
        auto* sleepStmt = new ASTNode(ctx.currentNode);
        sleepStmt->nodeType = ASTNodeType::AST_NODE; // TODO: Add SLEEP_STMT to AST
        sleepStmt->value = "sleep";
        ctx.currentNode->addChild(sleepStmt);
        ctx.currentNode = sleepStmt;
        ctx.state = STATE::STATEMENT_SLEEP;
        ctx.index--; // Re-process '('
    } else {
        // Not "sleep", treat as identifier
        ctx.stringStart = ctx.index - 4;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateStatementSleep(ParserContext& ctx, char c) {
    if (c == '(') {
        // Parse arguments
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        throw std::runtime_error("Expected '(' after sleep");
    }
}

// RawMemory handlers
inline void handleStateNoneR(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::NONE_RA;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRA(ParserContext& ctx, char c) {
    if (c == 'w') {
        ctx.state = STATE::NONE_RAW;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAW(ParserContext& ctx, char c) {
    if (c == 'M') {
        ctx.state = STATE::NONE_RAWM;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWM(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_RAWME;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 4;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWME(ParserContext& ctx, char c) {
    if (c == 'm') {
        ctx.state = STATE::NONE_RAWMEM;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 5;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWMEM(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_RAWMEMO;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 6;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWMEMO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_RAWMEMOR;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 7;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWMEMOR(ParserContext& ctx, char c) {
    if (c == 'y') {
        ctx.state = STATE::NONE_RAWMEMORY;
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 8;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneRAWMEMORY(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // RawMemory type
        ctx.state = STATE::TYPE_ANNOTATION;
        // Set type to RAW_MEMORY
        // This would need to be handled in the type annotation handler
    } else {
        // Not "RawMemory", treat as identifier
        ctx.stringStart = ctx.index - 9;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

// This handlers
inline void handleStateNoneTH(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_THI;
    } else {
        // Not "this", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneTHI(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::NONE_THIS;
    } else {
        // Not "this", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneTHIS(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c)) || c == '.' || c == '[' || c == '(') {
        // This expression
        auto* thisExpr = new ASTNode(ctx.currentNode);
        thisExpr->nodeType = ASTNodeType::AST_NODE; // TODO: Add THIS_EXPR to AST
        thisExpr->value = "this";
        ctx.currentNode->addChild(thisExpr);
        ctx.currentNode = thisExpr;
        ctx.state = STATE::EXPRESSION_THIS;
        ctx.index--; // Re-process this character
    } else {
        // Not "this", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExpressionThis(ParserContext& ctx, char c) {
    if (c == '.') {
        ctx.state = STATE::EXPRESSION_MEMBER_ACCESS;
    } else if (c == '[') {
        ctx.state = STATE::EXPRESSION_BRACKET_ACCESS;
    } else if (c == '(') {
        ctx.state = STATE::EXPRESSION_METHOD_CALL;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // End of this expression
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else {
        throw std::runtime_error("Unexpected character after 'this'");
    }
}

// New handlers
inline void handleStateNoneN(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_NE;
    } else {
        // Not "new", treat as identifier
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneNE(ParserContext& ctx, char c) {
    if (c == 'w') {
        ctx.state = STATE::NONE_NEW;
    } else {
        // Not "new", treat as identifier
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateNoneNEW(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // New expression
        auto* newExpr = new ASTNode(ctx.currentNode);
        newExpr->nodeType = ASTNodeType::AST_NODE; // TODO: Add NEW_EXPR to AST
        newExpr->value = "new";
        ctx.currentNode->addChild(newExpr);
        ctx.currentNode = newExpr;
        ctx.state = STATE::EXPRESSION_NEW;
    } else {
        // Not "new", treat as identifier
        ctx.stringStart = ctx.index - 3;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateExpressionNew(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Parse constructor call
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        ctx.index--; // Re-process this character
    }
}

// Expression handlers for member access, method calls, bracket access
inline void handleStateExpressionMemberAccess(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
    } else {
        throw std::runtime_error("Expected identifier after '.'");
    }
}

inline void handleStateExpressionMethodCall(ParserContext& ctx, char c) {
    if (c == '(') {
        // Parse arguments
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        throw std::runtime_error("Expected '(' for method call");
    }
}

inline void handleStateExpressionBracketAccess(ParserContext& ctx, char c) {
    if (c == '[') {
        // Parse index expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        throw std::runtime_error("Expected '[' for bracket access");
    }
}

// Increment handler
inline void handleStateExpressionIncrement(ParserContext& ctx, char c) {
    if (c == '+') {
        // ++ operator
        auto* incrementExpr = new ASTNode(ctx.currentNode);
        incrementExpr->nodeType = ASTNodeType::AST_NODE; // TODO: Add INCREMENT_EXPR to AST
        incrementExpr->value = "++";
        ctx.currentNode->addChild(incrementExpr);
        ctx.currentNode = incrementExpr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '+' for increment operator");
    }
}
