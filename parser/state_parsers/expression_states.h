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
    ctx.index--;
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
    } else if (c == '`') {
        // Template literal start
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL_START;
    } else if (c == '/') {
        // Regular expression start
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_REGEXP_START;
    } else if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::EXPRESSION_NUMBER;
    } else if (c == '-') {
        if (ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '-') {
            ctx.state = STATE::EXPRESSION_MINUS_MINUS;
        } else {
            ctx.state = STATE::EXPRESSION_UNARY_MINUS;
        }
    } else if (c == '+') {
        if (ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '+') {
            ctx.state = STATE::EXPRESSION_PLUS_PLUS;
        } else {
            ctx.state = STATE::EXPRESSION_UNARY_PLUS;
        }
    } else if (c == '!') {
        ctx.state = STATE::EXPRESSION_LOGICAL_NOT;
    } else if (c == '~') {
        ctx.state = STATE::EXPRESSION_BITWISE_NOT;
    } else if (c == '*') {
        if (ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '*') {
            ctx.state = STATE::EXPRESSION_EXPONENT;
        } else {
            throw std::runtime_error("Unexpected '*' in expression");
        }
    } else if (c == '&') {
        ctx.state = STATE::EXPRESSION_BIT_AND;
    } else if (c == '|') {
        ctx.state = STATE::EXPRESSION_BIT_OR;
    } else if (c == '^') {
        ctx.state = STATE::EXPRESSION_BIT_XOR;
    } else if (c == '<') {
        ctx.state = STATE::EXPRESSION_LEFT_SHIFT;
    } else if (c == '>') {
        ctx.state = STATE::EXPRESSION_RIGHT_SHIFT;
    } else if (c == '=') {
        ctx.state = STATE::EXPRESSION_EQUALS;
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

// Template literal handlers
inline void handleStateExpressionTemplateLiteralStart(ParserContext& ctx, char c) {
    auto* templateNode = new TemplateLiteralNode(ctx.currentNode);
    addExpressionOperand(ctx, templateNode);
    ctx.currentNode = templateNode;
    ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionTemplateLiteral(ParserContext& ctx, char c) {
    if (c == '\\') {
        ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL_ESCAPE;
    } else if (c == '$' && ctx.index + 1 < ctx.code.length() && ctx.code[ctx.index + 1] == '{') {
        // End of current quasi, start interpolation
        std::string quasi = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        static_cast<TemplateLiteralNode*>(ctx.currentNode)->addQuasi(quasi);
        ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL_INTERPOLATION;
        ctx.index++; // Skip the '{'
    } else if (c == '`') {
        // End of template literal
        std::string quasi = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        static_cast<TemplateLiteralNode*>(ctx.currentNode)->addQuasi(quasi);
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    }
}

inline void handleStateExpressionTemplateLiteralEscape(ParserContext& ctx, char c) {
    ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL;
}

inline void handleStateExpressionTemplateLiteralInterpolation(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of interpolation, back to template literal
        ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL;
    } else {
        // Parse expression inside interpolation
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

// Regular expression handlers
inline void handleStateExpressionRegExpStart(ParserContext& ctx, char c) {
    ctx.state = STATE::EXPRESSION_REGEXP;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionRegExp(ParserContext& ctx, char c) {
    if (c == '\\') {
        ctx.state = STATE::EXPRESSION_REGEXP_ESCAPE;
    } else if (c == '/') {
        // End of pattern, check for flags
        std::string pattern = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        ctx.state = STATE::EXPRESSION_REGEXP_FLAGS;
    }
}

inline void handleStateExpressionRegExpEscape(ParserContext& ctx, char c) {
    ctx.state = STATE::EXPRESSION_REGEXP;
}

inline void handleStateExpressionRegExpFlags(ParserContext& ctx, char c) {
    if (c >= 'a' && c <= 'z') {
        // Valid flag character, continue
        return;
    } else {
        // End of flags
        std::string pattern = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 1);
        std::string flags = ctx.code.substr(ctx.index, ctx.index - ctx.stringStart - pattern.length() - 2);
        auto* regexpNode = new RegExpLiteralNode(ctx.currentNode, pattern, flags);
        addExpressionOperand(ctx, regexpNode);
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}
