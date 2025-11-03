#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Root state handler - entry point for parsing
inline void handleStateNone(ParserContext& ctx, char c) {
    // Check if current node is a control statement that just finished parsing a single statement
    if (ctx.currentNode) {
        // Check for specific control statement types
        if (auto* ifNode = dynamic_cast<IfStatement*>(ctx.currentNode)) {
            // Check if this if statement needs its body set from the last parsed statement
            if (!ifNode->consequent && ctx.currentNode->children.size() > 1) {
                auto* lastChild = ctx.currentNode->children.back();
                ifNode->consequent = lastChild;
            }
            ctx.state = STATE::IF_ALTERNATE_START;
            return;
        } else if (auto* whileNode = dynamic_cast<WhileStatement*>(ctx.currentNode)) {
            // Check if this while statement needs its body set
            if (!whileNode->body && ctx.currentNode->children.size() > 1) {
                auto* lastChild = ctx.currentNode->children.back();
                whileNode->body = lastChild;
            }
            ctx.state = STATE::NONE;
            return;
        }
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (c == 'v') {
        ctx.state = STATE::NONE_V;
    } else if (c == 'c') {
        ctx.state = STATE::NONE_C;
    } else if (c == 'l') {
        ctx.state = STATE::NONE_L;
    } else if (c == 'f') {
        ctx.state = STATE::NONE_F;
    } else if (c == 'i') {
        ctx.state = STATE::NONE_I;
    } else if (c == 'w') {
        ctx.state = STATE::NONE_W;
    } else if (c == 'd') {
        ctx.state = STATE::NONE_D;
    } else if (c == 's') {
        ctx.state = STATE::NONE_S;
    } else if (c == 't') {
        ctx.state = STATE::NONE_T;
    } else {
        // Handle expression starts at top level
        auto* expr = new ExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    }
}
