#pragma once

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

#include "parser_context.h"
#include "ast.h"

inline void addExpressionOperand(ParserContext& ctx, ExpressionNode* operand) {
    if (!operand) {
        throw std::runtime_error("Operand node is null");
    }

    auto* currentNode = ctx.currentNode;
    if (currentNode->nodeType == ASTNodeType::BINARY_EXPRESSION) {
        auto* binaryNode = static_cast<BinaryExpressionNode*>(currentNode);
        if (binaryNode->children[0] == nullptr) {
            binaryNode->children[0] = operand;
            operand->parent = currentNode;
            ctx.currentNode = currentNode; // Stay on binary for operator application
        } else if (binaryNode->children[1] == nullptr) {
            binaryNode->children[1] = operand;
            operand->parent = currentNode;
            ctx.currentNode = currentNode; // Stay on binary for operator application
        } else {
            // This should not happen with proper operator precedence handling
            throw std::runtime_error("Binary expression already has two operands");
        }
    } else {
        currentNode->children.push_back(operand);
        operand->parent = currentNode;
        ctx.currentNode = operand;
    }
}

inline void applyExpressionOperator(ParserContext& ctx, BinaryExpressionOperator op) {
    auto* currentNode = ctx.currentNode;
    if (currentNode->nodeType != ASTNodeType::BINARY_EXPRESSION) {
        // Current node is an operand, create a new binary expression
        auto* originalParent = currentNode->parent;
        auto* newBinary = new BinaryExpressionNode(originalParent, op);
        newBinary->children[0] = currentNode;

        // Replace currentNode in parent's children
        if (originalParent) {
            auto& siblings = originalParent->children;
            auto it = std::find(siblings.begin(), siblings.end(), currentNode);
            if (it != siblings.end()) {
                *it = newBinary;
            }
        }

        currentNode->parent = newBinary;

        ctx.currentNode = newBinary;
    } else {
        auto* binaryNode = static_cast<BinaryExpressionNode*>(currentNode);
        if (binaryNode->children[0] == nullptr) {
            throw std::runtime_error("Missing left operand for operator");
        }
        if (binaryNode->op == BinaryExpressionOperator::OP_NULL) {
            binaryNode->op = op;
        } else {
            if (binaryNode->isNewOperatorGreaterPrecedence(op)) {
                auto* newBinary = new BinaryExpressionNode(binaryNode, op);
                newBinary->children[0] = binaryNode->right();
                binaryNode->children[1] = newBinary;
                ctx.currentNode = newBinary;
            } else {
                auto* newBinary = new BinaryExpressionNode(binaryNode->parent, op);
                newBinary->children[0] = binaryNode;
                binaryNode->parent = newBinary;

                // Update the parent's children to point to the new binary expression
                if (binaryNode->parent) {
                    auto& siblings = binaryNode->parent->children;
                    auto it = std::find(siblings.begin(), siblings.end(), binaryNode);
                    if (it != siblings.end()) {
                        *it = newBinary;
                    }
                }

                ctx.currentNode = newBinary;
            }
        }
    }
}

inline bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}

inline bool isIdentifierPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}
