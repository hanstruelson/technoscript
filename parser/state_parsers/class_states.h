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

// Class declaration state handlers
inline void handleStateNoneCL(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::NONE_CLA;
    } else {
        ctx.state = STATE::NONE;
        // Would need to include common_states.h for handleStateNone
        throw std::runtime_error("Unexpected character in 'cl' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneCLA(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::NONE_CLAS;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'cla' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneCLAS(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::NONE_CLASS;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Unexpected character in 'clas' sequence: " + std::string(1, c));
    }
}

inline void handleStateNoneCLASS(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Create class declaration node
        auto* classNode = new ClassDeclarationNode(ctx.currentNode);
        ctx.currentNode->children.push_back(classNode);
        ctx.currentNode = classNode;
        ctx.state = STATE::CLASS_DECLARATION_NAME;
    } else {
        ctx.state = STATE::NONE;
        throw std::runtime_error("Expected space after 'class': " + std::string(1, c));
    }
}

inline void handleStateClassDeclarationName(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        // Start of class name
        ctx.stringStart = ctx.index;
        // Stay in this state to continue parsing
    } else if (isIdentifierPart(c)) {
        // Continue parsing class name - accumulate characters
        return;
    } else if (c == '{') {
        // Class name complete (or anonymous class), extract it
        if (ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
        }
        ctx.state = STATE::CLASS_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Class name complete, extract it
        if (ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
        }
        // Stay in this state waiting for '{'
    } else if (c == 'e') {
        // Class name complete, extract it
        if (ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
        }
        // Check for 'extends'
        ctx.state = STATE::CLASS_EXTENDS_START;
        // Re-process this character
        ctx.index--;
    } else if (c == 'i') {
        // Class name complete, extract it
        if (ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
        }
        // Check for 'implements'
        ctx.state = STATE::CLASS_IMPLEMENTS_START;
        // Re-process this character
        ctx.index--;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected class name or '{', 'extends', or 'implements'", ctx.state);
    }
}

inline void handleStateClassExtendsStart(ParserContext& ctx, char c) {
    if (c == 'x') {
        // Continue checking 'extends'
        // This would need more states for full 'extends' parsing
        // For now, simplified - just skip to name
        ctx.state = STATE::CLASS_DECLARATION_NAME;
    } else {
        // Not 'extends', go back to name parsing
        ctx.state = STATE::CLASS_DECLARATION_NAME;
        handleStateClassDeclarationName(ctx, c);
    }
}

inline void handleStateClassImplementsStart(ParserContext& ctx, char c) {
    // Similar to extends, would need full parsing
    ctx.state = STATE::CLASS_DECLARATION_NAME;
    handleStateClassDeclarationName(ctx, c);
}

inline void handleStateClassBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::CLASS_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace and newlines
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '{' to start class body", ctx.state);
    }
}

inline void handleStateClassPropertyKey(ParserContext& ctx, char c);

inline void handleStateClassBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of class body
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else if (c == 's') {
        // Check for 'static'
        ctx.state = STATE::CLASS_STATIC_START;
    } else if (isIdentifierStart(c)) {
        // Property or method name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else if (std::isspace(static_cast<unsigned char>(c)) || c == ';') {
        // Skip whitespace and empty statements
    } else {
        reportParseError(ctx.code, ctx.index, "Expected property, method, or '}' in class body", ctx.state);
    }
}

inline void handleStateClassStaticStart(ParserContext& ctx, char c) {
    if (c == 't') {
        // Continue checking 'static'
        // For now, just skip ahead
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'static', treat as regular identifier
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        handleStateClassPropertyKey(ctx, c);
    }
}

inline void handleStateClassPropertyKey(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        // Continue reading identifier
        return;
    } else if (c == ':' || c == '=') {
        // End of property name
        std::string propName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* propNode = new ClassPropertyNode(ctx.currentNode);
        propNode->name = propName;
        static_cast<ClassDeclarationNode*>(ctx.currentNode)->addProperty(propNode);
        ctx.currentNode = propNode;

        if (c == ':') {
            ctx.state = STATE::CLASS_PROPERTY_TYPE;
        } else {
            ctx.state = STATE::CLASS_PROPERTY_INITIALIZER;
        }
    } else if (c == '(') {
        // This is a method (including constructor)
        std::string methodName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* methodNode = new ClassMethodNode(ctx.currentNode);
        methodNode->name = methodName;
        static_cast<ClassDeclarationNode*>(ctx.currentNode)->addMethod(methodNode);
        ctx.currentNode = methodNode;
        ctx.state = STATE::CLASS_METHOD_PARAMETERS_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue reading name
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or '(' after property/method name", ctx.state);
    }
}

inline void handleStateClassPropertyType(ParserContext& ctx, char c) {
    if (c == '=') {
        ctx.state = STATE::CLASS_PROPERTY_INITIALIZER;
    } else if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::CLASS_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Type annotation parsing would go here
        ctx.state = STATE::CLASS_PROPERTY_TYPE;
    }
}

inline void handleStateClassPropertyInitializer(ParserContext& ctx, char c) {
    if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::CLASS_BODY;
    } else {
        // Expression parsing would go here
        ctx.state = STATE::CLASS_PROPERTY_INITIALIZER;
    }
}

inline void handleStateClassMethodParametersStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::CLASS_METHOD_PARAMETERS_END;
    } else {
        // Parameter parsing would go here
        ctx.state = STATE::CLASS_METHOD_PARAMETERS_START;
    }
}

inline void handleStateClassMethodParametersEnd(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::CLASS_METHOD_RETURN_TYPE;
    } else if (c == '{') {
        ctx.state = STATE::CLASS_METHOD_BODY_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or '{' after method parameters", ctx.state);
    }
}

inline void handleStateClassMethodReturnType(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::CLASS_METHOD_BODY_START;
    } else {
        // Type parsing would go here
        ctx.state = STATE::CLASS_METHOD_RETURN_TYPE;
    }
}

inline void handleStateClassMethodBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::CLASS_METHOD_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '{' to start method body", ctx.state);
    }
}

inline void handleStateClassMethodBody(ParserContext& ctx, char c) {
    if (c == '}') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::CLASS_BODY;
    } else {
        // Method body parsing would go here
        ctx.state = STATE::CLASS_METHOD_BODY;
    }
}
