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
#include "expression_states.h"
#include "function_states.h"
#include "literal_states.h"
#include "control_flow_states.h"
#include "operator_states.h"
#include "class_states.h"
#include "interface_states.h"
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
