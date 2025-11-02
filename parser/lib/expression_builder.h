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
    ctx.currentNode = operand;
    if (currentNode->nodeType == ASTNodeType::BINARY_EXPRESSION) {
        auto* binaryNode = static_cast<BinaryExpressionNode*>(currentNode);
        if (binaryNode->children[0] == nullptr) {
            binaryNode->children[0] = operand;
        } else if (binaryNode->children[1] == nullptr) {
            binaryNode->children[1] = operand;
        } else {
            throw std::runtime_error("need to check order of operations");
        }
    } else {
        currentNode->children.push_back(operand);
        operand->parent = currentNode;
    }
}

inline void applyExpressionOperator(ParserContext& ctx, BinaryExpressionOperator op) {
    auto* currentNode = ctx.currentNode;
    if (currentNode->nodeType != ASTNodeType::BINARY_EXPRESSION) {
        throw std::runtime_error("Expected binary expression node for '+' operator");
    }
    auto* binaryNode = static_cast<BinaryExpressionNode*>(currentNode);
    if (binaryNode->children[0] == nullptr) {
        throw std::runtime_error("Missing left operand for '+' operator");
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
            ctx.currentNode = newBinary;
        }
    }
}

inline bool isIdentifierStart(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}

inline bool isIdentifierPart(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '$';
}
