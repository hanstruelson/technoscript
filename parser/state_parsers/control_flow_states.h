#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Control flow parsing states
inline void handleStateNoneI(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::IF_CONDITION_START;
    } else {
        throw std::runtime_error("Expected 'f' after 'i': " + std::string(1, c));
    }
}

inline void handleStateIfConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create if statement node and start parsing condition
        auto* ifNode = new IfStatement(ctx.currentNode);
        ctx.currentNode->children.push_back(ifNode);
        ctx.currentNode = ifNode;

        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        ifNode->condition = expr;
        ifNode->children.push_back(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'if': " + std::string(1, c));
    }
}

inline void handleStateIfConsequent(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* ifNode = dynamic_cast<IfStatement*>(ctx.currentNode)) {
            ifNode->consequent = block;
            ifNode->children.push_back(block);
        }
        ctx.currentNode = block;
        ctx.state = STATE::BLOCK_STATEMENT_BODY;  // Go directly to body since '{' is consumed
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - for now, just skip
        ctx.state = STATE::IF_ALTERNATE_START;
    }
}

inline void handleStateIfAlternateStart(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Check for "else"
        ctx.state = STATE::IF_ALTERNATE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // No else clause, end if statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}

inline void handleStateIfAlternate(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for else
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* ifNode = dynamic_cast<IfStatement*>(ctx.currentNode)) {
            ifNode->alternate = block;
            ifNode->children.push_back(block);
        }
        ctx.currentNode = block;
        ctx.state = STATE::BLOCK_STATEMENT_BODY;  // Go directly to body since '{' is consumed
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - for now, just skip
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}

// Block statement parsing states
inline void handleStateBlockStatementStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block start, wait for content or closing brace
        ctx.state = STATE::BLOCK_STATEMENT_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for block statement: " + std::string(1, c));
    }
}

inline void handleStateBlockStatementBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of block
        ctx.currentNode = ctx.currentNode->parent;
        // Check if this is the end of an if statement
        if (ctx.currentNode && ctx.currentNode->nodeType == ASTNodeType::IF_STATEMENT) {
            ctx.state = STATE::IF_ALTERNATE_START;
        } else {
            ctx.state = STATE::NONE;
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip block content - complex statement parsing needed
        return;
    }
}

// While loop parsing states
inline void handleStateNoneW(ParserContext& ctx, char c) {
    if (c == 'h') {
        ctx.state = STATE::NONE_WH;
    } else {
        throw std::runtime_error("Expected 'h' after 'w': " + std::string(1, c));
    }
}

inline void handleStateNoneWH(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_WHI;
    } else {
        throw std::runtime_error("Expected 'i' after 'wh': " + std::string(1, c));
    }
}

inline void handleStateNoneWHI(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::NONE_WHIL;
    } else {
        throw std::runtime_error("Expected 'l' after 'whi': " + std::string(1, c));
    }
}

inline void handleStateNoneWHIL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_WHILE;
    } else {
        throw std::runtime_error("Expected 'e' after 'whil': " + std::string(1, c));
    }
}

inline void handleStateNoneWHILE(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create while statement node and start parsing condition
        auto* whileNode = new WhileStatement(ctx.currentNode);
        ctx.currentNode->children.push_back(whileNode);
        ctx.currentNode = whileNode;

        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        whileNode->condition = expr;
        whileNode->children.push_back(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'while': " + std::string(1, c));
    }
}

inline void handleStateWhileConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* whileNode = dynamic_cast<WhileStatement*>(ctx.currentNode)) {
            whileNode->condition = expr;
            whileNode->children.push_back(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' for while condition: " + std::string(1, c));
    }
}

inline void handleStateWhileBody(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* whileNode = dynamic_cast<WhileStatement*>(ctx.currentNode)) {
            whileNode->body = block;
            whileNode->children.push_back(block);
        }
        ctx.currentNode = block;
        ctx.state = STATE::BLOCK_STATEMENT_BODY;  // Go directly to body since '{' is consumed
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - for now, just skip
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}
