#pragma once

#include <limits>
#include <stdexcept>
#include <string>
#include "../state.h"

class ASTNode;
class ExpressionNode;
class BinaryExpressionNode;

struct ExpressionFrame {
    ExpressionFrame* previous;
    ASTNode* owner;
    ExpressionNode* root;
    ExpressionNode* lastOperand;
    BinaryExpressionNode* rightmostBinary;

    ExpressionFrame(ExpressionFrame* prev, ASTNode* ownerNode)
        : previous(prev), owner(ownerNode), root(nullptr), lastOperand(nullptr), rightmostBinary(nullptr) {}
};

// you may NOT add any properties to this state! you must inspet the AST going up instead.
struct ParserContext {
    STATE state = STATE::NONE;
    ASTNode* root = nullptr;
    ASTNode* currentNode = nullptr;
    const std::string& code;
    size_t index = 0;
    size_t stringStart = std::numeric_limits<size_t>::max();
    char quoteChar = '\0'; // For string literals

    ParserContext(const std::string& codeRef, ASTNode* rootNode)
        : root(rootNode), currentNode(rootNode), code(codeRef) {}

    ~ParserContext() {
    }
};
