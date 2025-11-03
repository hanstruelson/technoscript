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
        // Check if we're closing an if or while condition
        auto* node = ctx.currentNode;
        while (node) {
            if (node->nodeType == ASTNodeType::IF_STATEMENT) {
                // Closing if condition, move to consequent
                ctx.currentNode = node;
                ctx.state = STATE::IF_CONSEQUENT;
                return false;
            } else if (node->nodeType == ASTNodeType::WHILE_STATEMENT) {
                // Closing while condition, move to body
                ctx.currentNode = node;
                ctx.state = STATE::WHILE_BODY;
                return false;
            }
            node = node->parent;
        }
        // Regular parenthesis closing
        ParenthesisExpressionNode::closeParenthesis(ctx);
    } else if (c == '+') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ADD);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '-') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_SUBTRACT);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '*') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_MULTIPLY);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '/') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_DIVIDE);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    }
    else if (c == ',') {
        // Check if we're in an array or object literal context
        auto* node = ctx.currentNode;
        while (node) {
            if (node->nodeType == ASTNodeType::ARRAY_LITERAL) {
                // Array element separator
                ctx.currentNode = node;
                ctx.state = STATE::ARRAY_LITERAL_SEPARATOR;
                return false;
            } else if (node->nodeType == ASTNodeType::OBJECT_LITERAL) {
                // Object property separator
                ctx.currentNode = node;
                ctx.state = STATE::OBJECT_LITERAL_SEPARATOR;
                return false;
            }
            node = node->parent;
        }
        return true; // Not handled
    } else if (c == ']') {
        // Check if we're in an array literal context
        auto* node = ctx.currentNode;
        while (node) {
            if (node->nodeType == ASTNodeType::ARRAY_LITERAL) {
                // End of array
                ctx.currentNode = node;
                ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
                return false;
            }
            node = node->parent;
        }
        return true; // Not handled
    } else if (c == '}') {
        // Check if we're in an object literal context
        auto* node = ctx.currentNode;
        while (node) {
            if (node->nodeType == ASTNodeType::OBJECT_LITERAL) {
                // End of object
                ctx.currentNode = node;
                ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
                return false;
            }
            node = node->parent;
        }
        return true; // Not handled
    } else if (c == ';') {
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
