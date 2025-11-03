#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"
#include "../lib/handle_post_operand.h"

// Define in order of dependency - handleStateNone and handleStateExpressionAfterOperand first
inline void handleStateNone(ParserContext& ctx, char c);
inline void handleStateExpressionAfterOperand(ParserContext& ctx, char c);

inline void ParenthesisExpressionNode::closeParenthesis(ParserContext& ctx) {
    auto* node = ctx.currentNode;
    if (node->nodeType == ASTNodeType::BINARY_EXPRESSION && node->children[1] == nullptr) {
        throw std::runtime_error("Missing right operand before ')'");
    }
    for (std::size_t i = 0; node && node->nodeType != ASTNodeType::PARENTHESIS_EXPRESSION; ++i) {
        node = node->parent;
        if (i > 1000) {
            throw std::runtime_error("Exceeded maximum parenthesis nesting level");
        }
    }
    if (!node) {
        throw std::runtime_error("Unexpected ')' while awaiting operand");
    }
    ctx.currentNode = node->parent;
}

inline void handleStateExpressionAfterOperand(ParserContext& ctx, char c) {
    if (c == '\n') {
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND_NEW_LINE;
    } else if (handlePostOperand(ctx, c)) {
        throw std::runtime_error(std::string("Unexpected character after operand: ") + c);
    }
}

inline void handleStateExpressionAfterOperandNewLine(ParserContext& ctx, char c) {
    if (handlePostOperand(ctx, c)) {
        return;
    }
    ctx.state = STATE::NONE;
    handleStateNone(ctx, c);
}

inline void handleStateExpressionExpectOperand(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }
    if (c == '(') {
        auto* newNode = new ParenthesisExpressionNode(ctx.currentNode);
        addExpressionOperand(ctx, newNode);
    } else if (c == ')') {
        ParenthesisExpressionNode::closeParenthesis(ctx);
    } else if (c == ';') {
        throw std::runtime_error("Missing expression before ';'");
    } else if (c == '\'') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_SINGLE_QUOTE;
    } else if (c == '"') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_DOUBLE_QUOTE;
    } else if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_NUMBER;
    } else if (c == '-') {
        ctx.state = STATE::EXPRESSION_MINUS;
    } else if (c == '+') {
        ctx.state = STATE::EXPRESSION_PLUS;
    } else if (c == '[') {
        // Array literal
        auto* arrayNode = new ArrayLiteralNode(ctx.currentNode);
        addExpressionOperand(ctx, arrayNode);
        ctx.currentNode = arrayNode;
        ctx.state = STATE::ARRAY_LITERAL_START;
    } else if (c == '{') {
        // Object literal
        auto* objectNode = new ObjectLiteralNode(ctx.currentNode);
        addExpressionOperand(ctx, objectNode);
        ctx.currentNode = objectNode;
        ctx.state = STATE::OBJECT_LITERAL_START;
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_IDENTIFIER;
    } else {
        throw std::runtime_error(std::string("Unexpected character in expression: ") + c);
    }
}

inline void handleStateExpressionNumber(ParserContext& ctx, char c) {
    if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        return;
    } else if (c == '.' && ctx.index > ctx.stringStart &&
        std::isdigit(static_cast<unsigned char>(ctx.code[ctx.index - 1])) != 0) {
        return;
    }

    std::string text = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    if (text.empty()) {
        throw std::runtime_error("Empty numeric literal");
    }
    auto* literal = new LiteralExpressionNode(nullptr, text);
    addExpressionOperand(ctx, literal);
    ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    handleStateExpressionAfterOperand(ctx, c);
}

inline void handleStateExpressionIdentifier(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        return;
    }

    std::string text = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    if (text.empty()) {
        throw std::runtime_error("Empty identifier");
    }
    auto* identifier = new IdentifierExpressionNode(nullptr, text);
    addExpressionOperand(ctx, identifier);
    ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    handleStateExpressionAfterOperand(ctx, c);
}

inline void handleStateExpressionSingleQuote(ParserContext& ctx, char c) {
    if (c == '\\') {
        ctx.state = STATE::EXPRESSION_SINGLE_QUOTE_ESCAPE;
    } else if (c == '\'') {
        if (ctx.index <= ctx.stringStart) {
            throw std::runtime_error("Invalid single-quoted literal bounds");
        }
        std::string value = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        auto* literal = new LiteralExpressionNode(nullptr, value);
        addExpressionOperand(ctx, literal);
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    }
}

inline void handleStateExpressionSingleQuoteEscape(ParserContext& ctx, char c) {
    if (c == '\n' || c == '\r') {
        ctx.state = STATE::EXPRESSION_SINGLE_QUOTE;
        return;
    }
    ctx.state = STATE::EXPRESSION_SINGLE_QUOTE;
}

inline void handleStateExpressionDoubleQuote(ParserContext& ctx, char c) {
    if (c == '\\') {
        ctx.state = STATE::EXPRESSION_DOUBLE_QUOTE_ESCAPE;
    } else if (c == '"') {
        if (ctx.index <= ctx.stringStart) {
            throw std::runtime_error("Invalid double-quoted literal bounds");
        }
        std::string value = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        auto* literal = new LiteralExpressionNode(nullptr, value);
        addExpressionOperand(ctx, literal);
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    }
}

inline void handleStateExpressionDoubleQuoteEscape(ParserContext& ctx, char) {
    ctx.state = STATE::EXPRESSION_DOUBLE_QUOTE;
}

inline void handleStateExpressionPlus(ParserContext& ctx, char c) {
    if (c == '+') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::IDENTIFIER_NAME;
        auto* plusPlusPrefixNode = new PlusPlusPrefixExpressionNode(nullptr);
        addExpressionOperand(ctx, plusPlusPrefixNode);
        ctx.state = STATE::IDENTIFIER_NAME;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ADD);
    } else {
        throw std::runtime_error("Expected '+' for '++' operator");
    }
}

inline void handleStateExpressionMinus(ParserContext& ctx, char c) {
    if (c == '-') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::IDENTIFIER_NAME;
        auto* minusMinusPrefixNode = new MinusMinusPrefixExpressionNode(nullptr);
        addExpressionOperand(ctx, minusMinusPrefixNode);
        ctx.state = STATE::IDENTIFIER_NAME;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_SUBTRACT);
    } else {
        throw std::runtime_error("Expected '-' for '--' operator");
    }
}
