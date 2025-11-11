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
        // Check if we just closed a print statement
        if (ctx.currentNode && ctx.currentNode->value == "print") {
            ctx.state = STATE::BLOCK;
            // Pop to block after print statement
            if (ctx.currentNode && ctx.currentNode->parent && ctx.currentNode->parent->nodeType == ASTNodeType::BLOCK_STATEMENT) {
                ctx.currentNode = ctx.currentNode->parent;
            }
        }
    } else if (c == '+') {
        // Check if this is postfix increment (++ after operand)
        if (ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '+') {
        // Postfix increment
        auto* postfixNode = new PlusPlusPostfixExpressionNode(ctx.currentNode);
        // Replace current operand with postfix node
        if (ctx.currentNode->parent && ctx.currentNode->parent->children.back() == ctx.currentNode) {
            ctx.currentNode->parent->children.back() = postfixNode;
        }
        postfixNode->children.push_back(ctx.currentNode);
        ctx.currentNode->parent = postfixNode;
        ctx.currentNode = postfixNode;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
        ctx.index += 1; // Skip the second '+'
        return false;
        } else {
            // Binary addition
            applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ADD);
            ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        }
    } else if (c == '-') {
        // Check if this is postfix decrement (-- after operand)
        if (ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '-') {
            // Postfix decrement
            auto* postfixNode = new MinusMinusPostfixExpressionNode(ctx.currentNode);
            // Replace current operand with postfix node
            if (ctx.currentNode->parent && ctx.currentNode->parent->children.back() == ctx.currentNode) {
                ctx.currentNode->parent->children.back() = postfixNode;
            }
            postfixNode->children.push_back(ctx.currentNode);
            ctx.currentNode->parent = postfixNode;
            ctx.currentNode = postfixNode;
            ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
            ctx.index++; // Skip the second '-'
            return false;
        } else {
            // Binary subtraction
            applyExpressionOperator(ctx, BinaryExpressionOperator::OP_SUBTRACT);
            ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        }
    } else if (c == '*') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_MULTIPLY);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '/') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_DIVIDE);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '%') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_MODULO);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '<') {
        ctx.state = STATE::EXPRESSION_LESS;
    } else if (c == '>') {
        ctx.state = STATE::EXPRESSION_GREATER;
    } else if (c == '=') {
        ctx.state = STATE::EXPRESSION_EQUALS;
    } else if (c == '!') {
        ctx.state = STATE::EXPRESSION_NOT;
    } else if (c == '&') {
        ctx.state = STATE::EXPRESSION_AND;
    } else if (c == '|') {
        ctx.state = STATE::EXPRESSION_OR;
    } else if (c == ',') {
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
        // Check if we're in template literal interpolation
        if (ctx.state == STATE::EXPRESSION_TEMPLATE_LITERAL_INTERPOLATION) {
            // End of interpolation, add the expression to template literal
            auto* templateNode = ctx.currentNode;
            while (templateNode && templateNode->nodeType != ASTNodeType::TEMPLATE_LITERAL) {
                templateNode = templateNode->parent;
            }
            if (templateNode) {
                // Add the current expression to the template literal
                // The current node should be an ExpressionNode
                if (ctx.currentNode && dynamic_cast<ExpressionNode*>(ctx.currentNode)) {
                    static_cast<TemplateLiteralNode*>(templateNode)->addExpression(static_cast<ExpressionNode*>(ctx.currentNode));
                }
                // Move current node back to template literal
                ctx.currentNode = templateNode;
                // Update stringStart for the next quasi
                ctx.stringStart = ctx.index;
            }
            ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL;
            return false;
        }
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

        // Check if we're parsing a single statement directly under a control statement
        if (ctx.currentNode && ctx.currentNode->parent) {
            auto* parent = ctx.currentNode->parent;
            if (parent->nodeType == ASTNodeType::IF_STATEMENT) {
                // We're parsing a single statement as consequent, move to alternate check
                ctx.state = STATE::IF_ALTERNATE_START;
                return false;
            } else if (parent->nodeType == ASTNodeType::WHILE_STATEMENT) {
                // End of while body single statement
                ctx.state = STATE::BLOCK;
                return false;
            } else if (parent->nodeType == ASTNodeType::FOR_STATEMENT) {
                // End of for body single statement
                ctx.state = STATE::BLOCK;
                return false;
            }
        }

        ctx.state = STATE::BLOCK;
        // Pop to block after statement completion
        if (ctx.currentNode && ctx.currentNode->parent && ctx.currentNode->parent->nodeType == ASTNodeType::BLOCK_STATEMENT) {
            ctx.currentNode = ctx.currentNode->parent;
        }
    } else {
        return true;
    }
    return false;
}
