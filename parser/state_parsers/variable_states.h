#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Define in order of dependency - handleStateExpectIdentifier first
inline void handleStateExpectIdentifier(ParserContext& ctx, char c);

// var keyword parsing - define in order of dependency
inline void handleStateNoneVAR(ParserContext& ctx, char c) {
    if (c == ' ') {
        auto* parent = ctx.currentNode;
        auto* variable = new VariableDefinitionNode(parent, VariableDefinitionType::VAR);
        parent->children.push_back(variable);
        ctx.currentNode = variable;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneVA(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_VAR;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneV(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::NONE_VA;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

// const keyword parsing
inline void handleStateNoneCONST(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Check if this is "const enum"
        std::size_t tempIndex = ctx.index;
        // Skip spaces
        while (tempIndex < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[tempIndex]))) {
            tempIndex++;
        }
        if (tempIndex + 3 < ctx.code.length() &&
            ctx.code[tempIndex] == 'e' &&
            ctx.code[tempIndex + 1] == 'n' &&
            ctx.code[tempIndex + 2] == 'u' &&
            ctx.code[tempIndex + 3] == 'm') {
            // This is "const enum"
            auto* enumDecl = new EnumDeclarationNode(ctx.currentNode);
            enumDecl->isConst = true;
            ctx.currentNode->addChild(enumDecl);
            ctx.currentNode = enumDecl;
            ctx.stringStart = 0;  // Reset for name parsing
            ctx.state = STATE::ENUM_DECLARATION_NAME;
            // Skip "enum"
            ctx.index = tempIndex + 4;
            return;
        } else {
            // Normal const variable
            auto* parent = ctx.currentNode;
            auto* variable = new VariableDefinitionNode(parent, VariableDefinitionType::CONST);
            parent->children.push_back(variable);
            ctx.currentNode = variable;
            ctx.state = STATE::EXPECT_IDENTIFIER;
        }
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneCONS(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_CONST;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneCON(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::NONE_CONS;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneCO(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::NONE_CON;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneC(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_CO;
    } else if (c == 'l') {
        ctx.state = STATE::NONE_CL;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

// let keyword parsing
inline void handleStateNoneLET(ParserContext& ctx, char c) {
    if (c == ' ') {
        auto* parent = ctx.currentNode;
        auto* variable = new VariableDefinitionNode(parent, VariableDefinitionType::LET);
        parent->children.push_back(variable);
        ctx.currentNode = variable;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneLE(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_LET;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}

inline void handleStateNoneL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_LE;
    } else {
        ctx.stringStart = ctx.index - 2;
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--;
    }
}
