#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Forward declarations
inline void handleStateExpressionExpectOperand(ParserContext& ctx, char c);
inline void handleStateArrayLiteralElement(ParserContext& ctx, char c);
inline void handleStateObjectLiteralPropertyKey(ParserContext& ctx, char c);

// Array literal parsing states
inline void handleStateArrayLiteralStart(ParserContext& ctx, char c) {
    if (c == ']') {
        // Empty array
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Start parsing first element
        ctx.state = STATE::ARRAY_LITERAL_ELEMENT;
        handleStateArrayLiteralElement(ctx, c);
    }
}

inline void handleStateArrayLiteralElement(ParserContext& ctx, char c) {
    if (c == ']') {
        // End of array
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (c == ',') {
        // Element separator
        ctx.state = STATE::ARRAY_LITERAL_SEPARATOR;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Parse element expression
        // Create a new expression context for the element
        auto* expr = new ExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        handleStateExpressionExpectOperand(ctx, c);
    }
}

inline void handleStateArrayLiteralSeparator(ParserContext& ctx, char c) {
    if (c == ']') {
        // End of array
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Parse next element
        ctx.state = STATE::ARRAY_LITERAL_ELEMENT;
        handleStateArrayLiteralElement(ctx, c);
    }
}

// Object literal parsing states
inline void handleStateObjectLiteralStart(ParserContext& ctx, char c) {
    if (c == '}') {
        // Empty object
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Start parsing first property
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_KEY;
        handleStateObjectLiteralPropertyKey(ctx, c);
    }
}

inline void handleStateObjectLiteralPropertyKey(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of object
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (isalpha(c) || c == '_') {
        // Property key (identifier) start
        ctx.stringStart = ctx.index;
        // Stay in this state to accumulate the key
    } else if (isalnum(c) || c == '_') {
        // Continue accumulating the key
        return;
    } else if (c == ':') {
        // Key complete, create property
        std::string key = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* property = new PropertyNode(ctx.currentNode);
        property->key = key;
        if (auto* obj = dynamic_cast<ObjectLiteralNode*>(ctx.currentNode)) {
            obj->addProperty(property);
        }
        ctx.currentNode = property;
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_VALUE;
    } else if (c == '"' || c == '\'') {
        // String key - for now, skip complex string parsing
        throw std::runtime_error("String property keys not yet supported");
    } else {
        throw std::runtime_error("Expected ':' after property key: " + std::string(1, c));
    }
}

inline void handleStateObjectLiteralPropertyKeyContinue(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue parsing property key
        return;
    } else if (c == ':') {
        // Key complete, expect colon
        std::string key = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* property = new PropertyNode(ctx.currentNode);
        property->key = key;
        if (auto* obj = dynamic_cast<ObjectLiteralNode*>(ctx.currentNode)) {
            obj->addProperty(property);
        }
        ctx.currentNode = property;
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_COLON;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Key complete, wait for colon
        std::string key = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* property = new PropertyNode(ctx.currentNode);
        property->key = key;
        if (auto* obj = dynamic_cast<ObjectLiteralNode*>(ctx.currentNode)) {
            obj->addProperty(property);
        }
        ctx.currentNode = property;
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_COLON;
    } else {
        throw std::runtime_error("Expected ':' after property key: " + std::string(1, c));
    }
}

inline void handleStateObjectLiteralPropertyColon(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_VALUE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ':' after property key: " + std::string(1, c));
    }
}

inline void handleStateObjectLiteralPropertyValue(ParserContext& ctx, char c) {
    if (c == ',' || c == '}') {
        // Property complete
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::OBJECT_LITERAL_SEPARATOR;
        } else {
            ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Parse property value expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* prop = dynamic_cast<PropertyNode*>(ctx.currentNode)) {
            prop->value = expr;
            prop->children.push_back(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        handleStateExpressionExpectOperand(ctx, c);
    }
}

inline void handleStateObjectLiteralSeparator(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of object
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::EXPRESSION_AFTER_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Parse next property
        ctx.state = STATE::OBJECT_LITERAL_PROPERTY_KEY;
        handleStateObjectLiteralPropertyKey(ctx, c);
    }
}
