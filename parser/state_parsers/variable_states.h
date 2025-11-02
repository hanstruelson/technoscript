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
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneVA(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_VAR;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneV(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::NONE_VA;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

// const keyword parsing
inline void handleStateNoneCONST(ParserContext& ctx, char c) {
    if (c == ' ') {
        auto* parent = ctx.currentNode;
        auto* variable = new VariableDefinitionNode(parent, VariableDefinitionType::CONST);
        parent->children.push_back(variable);
        ctx.currentNode = variable;
        ctx.state = STATE::EXPECT_IDENTIFIER;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneCONS(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_CONST;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneCON(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::NONE_CONS;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneCO(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::NONE_CON;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneC(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_CO;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
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
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneLE(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_LET;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateNoneL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_LE;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}
