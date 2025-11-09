#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"
#include "../state.h"

// Forward declaration for reportParseError
inline void reportParseError(const std::string& code, std::size_t index, const std::string& message, STATE state);

// Destructuring state handlers
inline void handleStateArrayDestructuringStart(ParserContext& ctx, char c) {
    if (c == ']') {
        // Empty array destructuring: []
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (c == ',') {
        // Empty element followed by comma
        ctx.state = STATE::ARRAY_DESTRUCTURING_SEPARATOR;
    } else if (c == '.') {
        // Check for rest element: ...
        ctx.state = STATE::ARRAY_DESTRUCTURING_REST;
    } else if (isIdentifierStart(c)) {
        // Variable name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::ARRAY_DESTRUCTURING_ELEMENT;
        ctx.index--; // Re-process this character
    } else if (c == '[') {
        // Nested array destructuring
        auto* nestedArray = new ArrayDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedArray);
        ctx.currentNode = nestedArray;
        ctx.state = STATE::ARRAY_DESTRUCTURING_START;
    } else if (c == '{') {
        // Nested object destructuring
        auto* nestedObject = new ObjectDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedObject);
        ctx.currentNode = nestedObject;
        ctx.state = STATE::OBJECT_DESTRUCTURING_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier, '[', '{', '...', or ']' in array destructuring", ctx.state);
    }
}

inline void handleStateArrayDestructuringElement(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        // Continue reading identifier
        return;
    } else if (c == ',') {
        // End of element
        std::string elementName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* arrayNode = dynamic_cast<ArrayDestructuringNode*>(ctx.currentNode)) {
            arrayNode->addElement(elementName);
        }
        ctx.state = STATE::ARRAY_DESTRUCTURING_SEPARATOR;
    } else if (c == ']') {
        // End of element and array
        std::string elementName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* arrayNode = dynamic_cast<ArrayDestructuringNode*>(ctx.currentNode)) {
            arrayNode->addElement(elementName);
        }
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (c == '=') {
        // Default value
        std::string elementName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* arrayNode = dynamic_cast<ArrayDestructuringNode*>(ctx.currentNode)) {
            arrayNode->addElement(elementName);
        }
        // For now, skip default value parsing
        ctx.state = STATE::ARRAY_DESTRUCTURING_ELEMENT;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ',', ']', '=', or identifier continuation in array destructuring element", ctx.state);
    }
}

inline void handleStateArrayDestructuringSeparator(ParserContext& ctx, char c) {
    if (c == ',') {
        // Another separator (empty element)
        return;
    } else if (c == ']') {
        // End of array
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (c == '.') {
        // Check for rest element
        ctx.state = STATE::ARRAY_DESTRUCTURING_REST;
    } else if (isIdentifierStart(c)) {
        // Next element
        ctx.stringStart = ctx.index;
        ctx.state = STATE::ARRAY_DESTRUCTURING_ELEMENT;
        ctx.index--; // Re-process this character
    } else if (c == '[') {
        // Nested array destructuring
        auto* nestedArray = new ArrayDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedArray);
        ctx.currentNode = nestedArray;
        ctx.state = STATE::ARRAY_DESTRUCTURING_START;
    } else if (c == '{') {
        // Nested object destructuring
        auto* nestedObject = new ObjectDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedObject);
        ctx.currentNode = nestedObject;
        ctx.state = STATE::OBJECT_DESTRUCTURING_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier, '[', '{', '...', ',', or ']' in array destructuring", ctx.state);
    }
}

inline void handleStateArrayDestructuringRest(ParserContext& ctx, char c) {
    if (c == '.') {
        // Second dot of ...
        return;
    } else if (isIdentifierStart(c)) {
        // Rest element name
        ctx.stringStart = ctx.index;
        // For now, just consume the identifier
        ctx.state = STATE::ARRAY_DESTRUCTURING_ELEMENT;
        ctx.index--; // Re-process this character
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier after '...' in array destructuring", ctx.state);
    }
}

inline void handleStateObjectDestructuringStart(ParserContext& ctx, char c) {
    if (c == '}') {
        // Empty object destructuring: {}
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (c == ',') {
        // Empty property followed by comma
        ctx.state = STATE::OBJECT_DESTRUCTURING_SEPARATOR;
    } else if (isIdentifierStart(c)) {
        // Property name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    } else if (c == '"' || c == '\'') {
        // Quoted property name
        ctx.quoteChar = c;
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_KEY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier, string, or '}' in object destructuring", ctx.state);
    }
}

inline void handleStateObjectDestructuringPropertyKey(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        // Continue reading identifier
        return;
    } else if (c == '"' || c == '\'') {
        // End of quoted string
        if (c == ctx.quoteChar) {
            std::string propertyName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* objectNode = dynamic_cast<ObjectDestructuringNode*>(ctx.currentNode)) {
                objectNode->addProperty(propertyName, propertyName); // key and value same
            }
            ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_COLON;
        }
    } else if (c == ':') {
        // End of property key
        std::string propertyName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* objectNode = dynamic_cast<ObjectDestructuringNode*>(ctx.currentNode)) {
            objectNode->addProperty(propertyName, propertyName); // key and value same for now
        }
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_VALUE;
    } else if (c == ',') {
        // Shorthand property
        std::string propertyName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* objectNode = dynamic_cast<ObjectDestructuringNode*>(ctx.currentNode)) {
            objectNode->addProperty(propertyName, propertyName);
        }
        ctx.state = STATE::OBJECT_DESTRUCTURING_SEPARATOR;
    } else if (c == '}') {
        // End of property and object
        std::string propertyName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* objectNode = dynamic_cast<ObjectDestructuringNode*>(ctx.currentNode)) {
            objectNode->addProperty(propertyName, propertyName);
        }
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':', ',', '}', or identifier continuation in object destructuring property", ctx.state);
    }
}

inline void handleStateObjectDestructuringPropertyColon(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_VALUE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' after property key in object destructuring", ctx.state);
    }
}

inline void handleStateObjectDestructuringPropertyValue(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        // Variable name for property
        ctx.stringStart = ctx.index;
        // For now, just consume the identifier
        ctx.state = STATE::OBJECT_DESTRUCTURING_SEPARATOR;
    } else if (c == '[') {
        // Nested array destructuring
        auto* nestedArray = new ArrayDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedArray);
        ctx.currentNode = nestedArray;
        ctx.state = STATE::ARRAY_DESTRUCTURING_START;
    } else if (c == '{') {
        // Nested object destructuring
        auto* nestedObject = new ObjectDestructuringNode(ctx.currentNode);
        ctx.currentNode->children.push_back(nestedObject);
        ctx.currentNode = nestedObject;
        ctx.state = STATE::OBJECT_DESTRUCTURING_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier, '[', or '{' for property value in object destructuring", ctx.state);
    }
}

inline void handleStateObjectDestructuringSeparator(ParserContext& ctx, char c) {
    if (c == ',') {
        // Next property
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_KEY;
    } else if (c == '}') {
        // End of object
        ctx.currentNode = ctx.currentNode->parent;
        // Check if we're in a function parameter context
        if (dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            ctx.state = STATE::FUNCTION_PARAMETER_COMPLETE;
        } else {
            ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        }
    } else if (isIdentifierStart(c)) {
        // Next property name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::OBJECT_DESTRUCTURING_PROPERTY_KEY;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ',' or '}' in object destructuring", ctx.state);
    }
}

inline void handleStateObjectDestructuringRest(ParserContext& ctx, char c) {
    if (c == '.') {
        // Second dot of ...
        return;
    } else if (isIdentifierStart(c)) {
        // Rest element name
        ctx.stringStart = ctx.index;
        // For now, just consume the identifier
        ctx.state = STATE::OBJECT_DESTRUCTURING_SEPARATOR;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier after '...' in object destructuring", ctx.state);
    }
}
