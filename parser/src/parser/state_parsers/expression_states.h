#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"
#include "../lib/handle_post_operand.h"

// Define in order of dependency - handleStateBlock and handleStateExpressionAfterOperand first
inline void handleStateBlock(ParserContext& ctx, char c);
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
    // Check if we're parsing an enum member initializer and see ',' or '}'
    auto* enumMemberNode = ctx.currentNode;
    while (enumMemberNode && enumMemberNode->nodeType != ASTNodeType::ENUM_MEMBER) {
        enumMemberNode = enumMemberNode->parent;
    }
    if ((c == ',' || c == '}') && enumMemberNode) {
        auto* enumMember = static_cast<EnumMemberNode*>(enumMemberNode);
        // The current expression should be the initializer
        if (enumMember->children.size() == 1 && enumMember->children[0]->nodeType == ASTNodeType::EXPRESSION) {
            auto* expr = static_cast<ExpressionNode*>(enumMember->children[0]);
            if (expr->children.size() == 1) {
                enumMember->initializer = static_cast<ExpressionNode*>(expr->children[0]);
                // Remove from expression wrapper
                expr->children.clear();
            }
        }
        // Move back to enum declaration
        ctx.currentNode = enumMemberNode->parent;
        if (c == ',') {
            ctx.state = STATE::ENUM_MEMBER_SEPARATOR;
        } else {
            // End of enum
            ctx.state = STATE::BLOCK;
        }
        // Re-process this character in the new state
        ctx.index--;
        return;
    }

    // Check if we're parsing a parameter pattern and see ':'
    if (c == ':' && ctx.currentNode && ctx.currentNode->parent &&
        ctx.currentNode->parent->nodeType == ASTNodeType::PARAMETER) {
        auto* param = static_cast<ParameterNode*>(ctx.currentNode->parent);
        if (!param->pattern) {
            // We're parsing the parameter pattern
            param->pattern = ctx.currentNode;
        }
        // Move back to parameter
        ctx.currentNode = param;
        ctx.state = STATE::FUNCTION_PARAMETER_TYPE_ANNOTATION;
        return;
    }

    // Check if we're parsing a parameter pattern or default value and see ',' or ')'
    if ((c == ',' || c == ')') && ctx.currentNode && ctx.currentNode->parent &&
        ctx.currentNode->parent->nodeType == ASTNodeType::PARAMETER) {
        auto* param = static_cast<ParameterNode*>(ctx.currentNode->parent);
        if (!param->pattern) {
            // We're parsing the parameter pattern
            param->pattern = ctx.currentNode;
        } else {
            // We're parsing a default value
            param->defaultValue = ctx.currentNode;
        }
        // Move back to parameter list
        ctx.currentNode = param->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
        return;
    }

    // Check if we're inside template literal interpolation and see '}'
    if (c == '}') {
        auto* node = ctx.currentNode;
        while (node) {
            if (node->nodeType == ASTNodeType::TEMPLATE_LITERAL) {
                // We're inside template literal, treat '}' as end of interpolation
                auto* templateNode = node;
                // Add the current expression to the template literal
                if (ctx.currentNode && dynamic_cast<ExpressionNode*>(ctx.currentNode)) {
                    static_cast<TemplateLiteralNode*>(templateNode)->addExpression(static_cast<ExpressionNode*>(ctx.currentNode));
                }
                // Move current node back to template literal
                ctx.currentNode = templateNode;
                // Update stringStart for the next quasi
                ctx.stringStart = ctx.index;
                ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL;
                return;
            }
            node = node->parent;
        }
    }

    // Check if the current node is an identifier "await" - if so, convert to await expression
    if (ctx.currentNode && ctx.currentNode->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
        auto* identNode = static_cast<IdentifierExpressionNode*>(ctx.currentNode);
        if (identNode->name == "await") {
            // Convert identifier to await expression
            auto* awaitNode = new AwaitExpressionNode(ctx.currentNode->parent);
            // Replace in parent's children
            if (ctx.currentNode->parent) {
                auto& siblings = ctx.currentNode->parent->children;
                auto it = std::find(siblings.begin(), siblings.end(), ctx.currentNode);
                if (it != siblings.end()) {
                    *it = awaitNode;
                }
            }
            // Don't delete the identifier node, just replace it
            ctx.currentNode = awaitNode;
            // Set state to expect the awaited expression
            ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
            // Re-process this character
            ctx.index--;
            return;
        }
    }

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
    ctx.state = STATE::BLOCK;
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
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPRESSION_SINGLE_QUOTE;
    } else if (c == '"') {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPRESSION_DOUBLE_QUOTE;
    } else if (c == '`') {
        // Template literal start
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPRESSION_TEMPLATE_LITERAL_START;
    } else if (c == '/') {
        // Regular expression start
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPRESSION_REGEXP_START;
    } else if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        ctx.stringStart = ctx.index - 1;
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
        ctx.stringStart = ctx.index - 1;
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

    std::string text = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
    if (text.empty()) {
        throw std::runtime_error("Empty numeric literal");
    }
    auto* literal = new LiteralExpressionNode(nullptr, text);
    addExpressionOperand(ctx, literal);
    ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    ctx.index--;
}

inline void handleStateExpressionIdentifier(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        return;
    }

    std::string text = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
    // Trim trailing whitespace
    text.erase(text.find_last_not_of(" \t\n\r\f\v") + 1);
    if (text.empty()) {
        throw std::runtime_error("Empty identifier");
    }
    auto* identifier = new IdentifierExpressionNode(nullptr, text);
    addExpressionOperand(ctx, identifier);
    ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    ctx.index--;
}

inline void handleStateExpressionSingleQuote(ParserContext& ctx, char c) {
    if (c == '\\') {
        ctx.state = STATE::EXPRESSION_SINGLE_QUOTE_ESCAPE;
    } else if (c == '\'') {
        if (ctx.index <= ctx.stringStart) {
            throw std::runtime_error("Invalid single-quoted literal bounds");
        }
        std::string value = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 2);
        auto* literal = new LiteralExpressionNode(nullptr, value, DataType::STRING);
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
        std::string value = ctx.code.substr(ctx.stringStart + 1, ctx.index - ctx.stringStart - 2);
        auto* literal = new LiteralExpressionNode(nullptr, value, DataType::STRING);
        addExpressionOperand(ctx, literal);
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    }
}

inline void handleStateExpressionDoubleQuoteEscape(ParserContext& ctx, char) {
    ctx.state = STATE::EXPRESSION_DOUBLE_QUOTE;
}

inline void handleStateExpressionPlus(ParserContext& ctx, char c) {
    if (c == '+') {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        auto* plusPlusPrefixNode = new PlusPlusPrefixExpressionNode(nullptr);
        addExpressionOperand(ctx, plusPlusPrefixNode);
        ctx.state = STATE::IDENTIFIER_NAME;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ADD);
    } else {
        throw std::runtime_error("Expected '+' for '++' operator");
    }
}

inline void handleStateExpressionMinus(ParserContext& ctx, char c) {
    if (c == '-') {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::IDENTIFIER_NAME;
        auto* minusMinusPrefixNode = new MinusMinusPrefixExpressionNode(nullptr);
        addExpressionOperand(ctx, minusMinusPrefixNode);
        ctx.state = STATE::IDENTIFIER_NAME;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index - 1;
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
    } else if (c == '$' && ctx.index < ctx.code.length() && ctx.code[ctx.index] == '{') {
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
