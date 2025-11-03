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

// Interface declaration state handlers
inline void handleStateNoneIN(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_INT;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'in' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINT(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_INTE;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'int' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTE(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_INTER;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'inte' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTER(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::NONE_INTERF;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'inter' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTERF(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::NONE_INTERFA;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'interf' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTERFA(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::NONE_INTERFAC;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'interfa' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTERFAC(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_INTERFACE;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'interfac' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneINTERFACE(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Create interface declaration node
        auto* interfaceNode = new InterfaceDeclarationNode(ctx.currentNode);
        ctx.currentNode->children.push_back(interfaceNode);
        ctx.currentNode = interfaceNode;
        ctx.state = STATE::INTERFACE_DECLARATION_NAME;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Expected space after 'interface': " + std::string(1, c));
    }
}

inline void handleStateInterfaceDeclarationName(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        // Start of interface name
        ctx.stringStart = ctx.index;
        // Stay in this state to continue parsing
    } else if (isIdentifierPart(c)) {
        // Continue parsing interface name - accumulate characters
        return;
    } else if (c == '{') {
        // Interface name complete, extract it
        if (ctx.stringStart < ctx.index) {
            std::string interfaceName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->name = interfaceName;
            }
        }
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Interface name complete, extract it
        if (ctx.stringStart < ctx.index) {
            std::string interfaceName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->name = interfaceName;
            }
        }
        // Stay in this state waiting for '{'
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name or '{'", ctx.state);
    }
}

inline void handleStateInterfaceBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace and newlines
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '{' to start interface body", ctx.state);
    }
}

inline void handleStateInterfaceBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of interface body
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else if (isIdentifierStart(c)) {
        // Property or method name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
    } else if (std::isspace(static_cast<unsigned char>(c)) || c == ';') {
        // Skip whitespace and empty statements
    } else {
        reportParseError(ctx.code, ctx.index, "Expected property, method, or '}' in interface body", ctx.state);
    }
}

inline void handleStateInterfacePropertyKey(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        // Continue reading identifier
        return;
    } else if (c == ':' || c == '(') {
        // End of property/method name
        std::string propName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);

        if (c == ':') {
            // This is a property
            auto* propNode = new PropertyNode(ctx.currentNode);
            propNode->key = propName;
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->addProperty(propNode);
            }
            ctx.currentNode = propNode;
            ctx.state = STATE::INTERFACE_PROPERTY_TYPE;
        } else {
            // This is a method
            // For interfaces, methods don't have bodies, just signatures
            // We'll create a simple AST node to represent the method signature
            auto* methodNode = new ASTNode(ctx.currentNode);
            methodNode->nodeType = ASTNodeType::INTERFACE_METHOD;
            methodNode->value = propName;
            ctx.currentNode->children.push_back(methodNode);
            ctx.currentNode = methodNode;
            ctx.state = STATE::INTERFACE_METHOD_PARAMETERS_START;
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue reading name
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or '(' after property/method name", ctx.state);
    }
}

inline void handleStateInterfacePropertyType(ParserContext& ctx, char c) {
    if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Type annotation parsing would go here
        ctx.state = STATE::INTERFACE_PROPERTY_TYPE;
    }
}

inline void handleStateInterfaceMethodParametersStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::INTERFACE_METHOD_PARAMETERS_END;
    } else {
        // Parameter parsing would go here
        ctx.state = STATE::INTERFACE_METHOD_PARAMETERS_START;
    }
}

inline void handleStateInterfaceMethodParametersEnd(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::INTERFACE_METHOD_RETURN_TYPE;
    } else if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or ';' after method parameters", ctx.state);
    }
}

inline void handleStateInterfaceMethodReturnType(ParserContext& ctx, char c) {
    if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else {
        // Type parsing would go here
        ctx.state = STATE::INTERFACE_METHOD_RETURN_TYPE;
    }
}
