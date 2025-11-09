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
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Check if we've started parsing the class name
        if (ctx.stringStart > 0) {
            // Class name complete, extract it
            std::string className = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
            ctx.state = STATE::CLASS_AFTER_NAME_START;
        } else {
            // Skip whitespace before class name starts
            return;
        }
    } else if (ctx.stringStart == 0 && isIdentifierStart(c)) {
        // Start of class name
        ctx.stringStart = ctx.index - 1;
        // Stay in this state to continue parsing
    } else if (isIdentifierPart(c)) {
        // Continue parsing class name - accumulate characters
        return;
    } else if (c == '<') {
        // Generic parameters start
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                classNode->name = className;
            }
        }
        ctx.state = STATE::CLASS_GENERIC_PARAMETERS_START;
        ctx.index--; // Re-process this character
    } else if (c == '{') {
        // Class name complete (or anonymous class), extract it
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string className = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
                if (classNode->name.empty()) {
                    classNode->name = className;
                }
            }
        }
        ctx.state = STATE::CLASS_BODY;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected class name, '<', or '{', 'extends', or 'implements'", ctx.state);
    }
}

inline void handleStateClassExtendsStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // No extends/implements, go to body
        ctx.state = STATE::CLASS_BODY;
    } else if (c == 'e') {
        ctx.state = STATE::CLASS_AFTER_NAME_E;
    } else if (c == 'i') {
        ctx.state = STATE::CLASS_INHERITANCE_I;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Unexpected character
        reportParseError(ctx.code, ctx.index, "Expected 'extends', 'implements', or '{' after class name", ctx.state);
    }
}

inline void handleStateClassExtendsName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building class name
        return;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // End of class name
        std::string extendsClass = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classNode->extendsClass = extendsClass;
        }
        // Check for 'implements' or go to body
        ctx.state = STATE::CLASS_IMPLEMENTS_START;
    } else if (c == '{') {
        // End of class name, go to body
        std::string extendsClass = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classNode->extendsClass = extendsClass;
        }
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    } else {
        reportParseError(ctx.code, ctx.index, "Expected class name after 'extends'", ctx.state);
    }
}

inline void handleStateClassImplementsStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // No implements, go to body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    } else if (c == 'i') {
        ctx.state = STATE::CLASS_INHERITANCE_I;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Unexpected
        reportParseError(ctx.code, ctx.index, "Expected 'implements' or '{'", ctx.state);
    }
}

inline void handleStateClassImplementsName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building interface name
        return;
    } else if (c == ',') {
        // End of interface name, add to implements list
        std::string interfaceName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classNode->implementsInterfaces.push_back(interfaceName);
        }
        ctx.state = STATE::CLASS_IMPLEMENTS_SEPARATOR;
    } else if (c == '{') {
        // End of interface name, add to implements list, go to body
        std::string interfaceName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classNode->implementsInterfaces.push_back(interfaceName);
        }
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name after 'implements'", ctx.state);
    }
}

inline void handleStateClassImplementsSeparator(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Next interface name
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_IMPLEMENTS_NAME;
        ctx.index--; // Re-process this character
    } else if (c == '{') {
        // End of implements, go to body
        ctx.state = STATE::CLASS_BODY;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name or '{' after ','", ctx.state);
    }
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
    } else if (c == 'p') {
        // Check for 'public'
        ctx.state = STATE::CLASS_ACCESS_MODIFIER_PUBLIC;
    } else if (c == 'r') {
        // Check for 'readonly' or 'private'
        ctx.state = STATE::CLASS_READONLY_MODIFIER;
    } else if (c == 'a') {
        // Check for 'abstract'
        ctx.state = STATE::CLASS_ABSTRACT_MODIFIER;
    } else if (c == 'g') {
        // Check for 'get'
        ctx.state = STATE::CLASS_GETTER_START;
    } else if (c == 'S') {
        // Check for 'set'
        ctx.state = STATE::CLASS_SETTER_START;
    } else if (isIdentifierStart(c)) {
        // Property or method name
        ctx.stringStart = ctx.index - 1; // ctx.index was already incremented, so subtract 1
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
        ctx.stringStart = ctx.index - 2; // 's' was at index-2, 't' is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'static', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 's' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
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
        ctx.state = STATE::CLASS_METHOD_BODY;
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
    if (c == '}') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::CLASS_BODY;
    } else {
        ctx.state = STATE::CLASS_METHOD_BODY;
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

// New handler functions for class features
inline void handleStateClassAccessModifierPublic(ParserContext& ctx, char c) {
    if (c == 'u') {
        // Continue checking 'public'
        // For now, just set a flag and continue
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else if (c == 'r') {
        // Check for 'private'
        ctx.state = STATE::CLASS_ACCESS_MODIFIER_PRIVATE;
    } else {
        // Not 'public' or 'private', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'p' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassAccessModifierPrivate(ParserContext& ctx, char c) {
    if (c == 'i') {
        // Continue checking 'private'
        // For now, just set a flag and continue
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'private', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'p' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassAccessModifierProtected(ParserContext& ctx, char c) {
    if (c == 'o') {
        // Continue checking 'protected'
        // For now, just set a flag and continue
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'protected', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'p' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassReadonlyModifier(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Continue checking 'readonly'
        // For now, just set a flag and continue
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'readonly', check for 'private'
        ctx.state = STATE::CLASS_ACCESS_MODIFIER_PRIVATE;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassAbstractModifier(ParserContext& ctx, char c) {
    if (c == 'b') {
        // Continue checking 'abstract'
        // For now, just set a flag and continue
        ctx.state = STATE::CLASS_PROPERTY_KEY;
    } else {
        // Not 'abstract', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'a' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassGetterStart(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Continue checking 'get'
        ctx.state = STATE::CLASS_GETTER_NAME;
    } else {
        // Not 'get', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'g' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassSetterStart(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Continue checking 'set'
        ctx.state = STATE::CLASS_SETTER_NAME;
    } else {
        // Not 'set', treat as regular identifier
        ctx.stringStart = ctx.index - 2; // 'S' was at index-2, current char is at index-1
        ctx.state = STATE::CLASS_PROPERTY_KEY;
        ctx.index--; // Re-process current character
    }
}

inline void handleStateClassGetterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (isIdentifierStart(c)) {
        // Start of getter name
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::CLASS_GETTER_PARAMETERS_START;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected getter name", ctx.state);
    }
}

inline void handleStateClassSetterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (isIdentifierStart(c)) {
        // Start of setter name
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::CLASS_SETTER_PARAMETERS_START;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected setter name", ctx.state);
    }
}

inline void handleStateClassGetterParametersStart(ParserContext& ctx, char c) {
    if (c == '(') {
        ctx.state = STATE::CLASS_GETTER_BODY_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '(' after getter name", ctx.state);
    }
}

inline void handleStateClassSetterParametersStart(ParserContext& ctx, char c) {
    if (c == '(') {
        ctx.state = STATE::CLASS_SETTER_BODY_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '(' after setter name", ctx.state);
    }
}

inline void handleStateClassGetterBodyStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::CLASS_GETTER_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ')' in getter parameters", ctx.state);
    }
}

inline void handleStateClassSetterBodyStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::CLASS_SETTER_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ')' in setter parameters", ctx.state);
    }
}

inline void handleStateClassGetterBody(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::CLASS_GETTER_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '{' to start getter body", ctx.state);
    }
}

inline void handleStateClassSetterBody(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::CLASS_SETTER_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '{' to start setter body", ctx.state);
    }
}

// Class generic parameter parsing: class Name<T, U>
inline void handleStateClassGenericParametersStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '<') {
        // Create generic type parameters node
        auto* genericParams = new GenericTypeParametersNode(ctx.currentNode);
        if (auto* classNode = dynamic_cast<ClassDeclarationNode*>(ctx.currentNode)) {
            classNode->genericParameters = genericParams;
        }
        ctx.currentNode->children.push_back(genericParams);
        ctx.currentNode = genericParams;
        ctx.state = STATE::CLASS_GENERIC_PARAMETER_NAME;
    } else {
        throw std::runtime_error("Expected '<' for class generic type parameters, got: " + std::string(1, c));
    }
}

inline void handleStateClassGenericParameterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::CLASS_GENERIC_PARAMETER_SEPARATOR;
        return;
    }

    throw std::runtime_error("Expected identifier for class generic type parameter, got: " + std::string(1, c));
}

inline void handleStateClassGenericParameterSeparator(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == ',') {
        // Add the parameter name
        std::string paramName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        paramName.erase(paramName.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* genericParams = dynamic_cast<GenericTypeParametersNode*>(ctx.currentNode);
        if (!genericParams) {
            throw std::runtime_error("Expected GenericTypeParametersNode");
        }
        genericParams->addParameter(paramName);

        // Continue with next parameter
        ctx.state = STATE::CLASS_GENERIC_PARAMETER_NAME;
        return;
    }

    if (c == '>') {
        // Add the last parameter name
        std::string paramName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        paramName.erase(paramName.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* genericParams = dynamic_cast<GenericTypeParametersNode*>(ctx.currentNode);
        if (!genericParams) {
            throw std::runtime_error("Expected GenericTypeParametersNode");
        }
        genericParams->addParameter(paramName);

        // Move back to parent and continue with class declaration
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::CLASS_AFTER_NAME_START;
        return;
    }

    throw std::runtime_error("Expected ',' or '>' in class generic type parameters, got: " + std::string(1, c));
}

inline void handleStateClassGenericParametersEnd(ParserContext& ctx, char c) {
    // This state is not currently used but is included for completeness
    // The logic is handled in CLASS_GENERIC_PARAMETER_SEPARATOR
    throw std::runtime_error("Unexpected state: CLASS_GENERIC_PARAMETERS_END");
}
