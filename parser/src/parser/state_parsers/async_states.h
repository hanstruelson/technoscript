#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Async/await state handlers

// "a" keyword detection
inline void handleStateBlockA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::BLOCK_AS;
    } else if (c == 'w') {
        ctx.state = STATE::BLOCK_AW;
    } else {
        throw std::runtime_error("Unexpected character after 'a': " + std::string(1, c));
    }
}

// "as" keyword continuation
inline void handleStateBlockAS(ParserContext& ctx, char c) {
    if (c == 'y') {
        ctx.state = STATE::BLOCK_ASY;
    } else {
        throw std::runtime_error("Expected 'y' after 'as': " + std::string(1, c));
    }
}

// "asy" keyword continuation
inline void handleStateBlockASY(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::BLOCK_ASYN;
    } else {
        throw std::runtime_error("Expected 'n' after 'asy': " + std::string(1, c));
    }
}

// "asyn" keyword continuation
inline void handleStateBlockASYN(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::BLOCK_ASYN;
    } else {
        throw std::runtime_error("Expected 'c' after 'asyn': " + std::string(1, c));
    }
}

// "async" keyword completion
inline void handleStateBlockASYNC(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Async function declaration
        auto* funcNode = new FunctionDeclarationNode(ctx.currentNode);
        funcNode->isAsync = true; // We'd need to add this field to FunctionDeclarationNode
        ctx.currentNode->children.push_back(funcNode);
        ctx.currentNode = funcNode;
        ctx.state = STATE::FUNCTION_DECLARATION_NAME;
    } else {
        throw std::runtime_error("Expected ' ' after 'async': " + std::string(1, c));
    }
}

// "aw" keyword continuation
inline void handleStateBlockAW(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::BLOCK_AWA;
    } else {
        throw std::runtime_error("Expected 'a' after 'aw': " + std::string(1, c));
    }
}

// "awa" keyword continuation
inline void handleStateBlockAWA(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::BLOCK_AWAI;
    } else {
        throw std::runtime_error("Expected 'i' after 'awa': " + std::string(1, c));
    }
}

// "awai" keyword continuation
inline void handleStateBlockAWAI(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::BLOCK_AWAIT;
    } else {
        throw std::runtime_error("Expected 't' after 'awai': " + std::string(1, c));
    }
}

// "await" keyword completion
inline void handleStateBlockAWAIT(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Await expression
        ctx.state = STATE::EXPRESSION_AWAIT;
    } else {
        throw std::runtime_error("Expected ' ' after 'await': " + std::string(1, c));
    }
}

// Await expression
inline void handleStateExpressionAwait(ParserContext& ctx, char c) {
    // Create an await expression node
    auto* awaitNode = new AwaitExpressionNode(ctx.currentNode);
    ctx.currentNode->children.push_back(awaitNode);
    ctx.currentNode = awaitNode;

    // Await is followed by an expression
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character as the start of the awaited expression
    ctx.index--;
}
