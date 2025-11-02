#pragma once

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "expression_builder.h"
#include "close_expression.h"
#include "ast.h"

void handleStateExpressionPlus(ParserContext& ctx, char c);
void handleStateExpressionMinus(ParserContext& ctx, char c);

inline bool handlePostOperand(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        
    } else if (c == ')') {
        ParenthesisExpressionNode::closeParenthesis(ctx);
    } else if (c == '+') {
        handleStateExpressionPlus(ctx, c);
    } else if (c == '-') {
        handleStateExpressionMinus(ctx, c);
    }
    // else if (c == '*') {
    //     handleStateExpressionAsterisk(ctx, BinaryExpressionOperator::OP_MULTIPLY);
    // } else if (c == '/') {
    //     handleStateExpressionSlash(ctx, BinaryExpressionOperator::OP_DIVIDE);
    // } 
    else if (c == ';') {
        closeExpression(ctx, c);
        if (ctx.currentNode && ctx.currentNode->parent) {
            ctx.currentNode = ctx.currentNode->parent;
        }
        ctx.state = STATE::NONE;
    } else {
        return true;
    }
    return false;
}
