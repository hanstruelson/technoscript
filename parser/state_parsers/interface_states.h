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
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Check if we've started parsing the interface name
        if (ctx.stringStart > 0) {
            // Interface name complete, extract it
            std::string interfaceName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->name = interfaceName;
            }
            ctx.state = STATE::INTERFACE_EXTENDS_START;
        } else {
            // Skip whitespace before interface name starts
            return;
        }
    } else if (ctx.stringStart == 0 && isIdentifierStart(c)) {
        // Start of interface name
        ctx.stringStart = ctx.index - 1;
        // Stay in this state to continue parsing
    } else if (isIdentifierPart(c)) {
        // Continue parsing interface name - accumulate characters
        return;
    } else if (c == '<') {
        // Generic parameters start
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string interfaceName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->name = interfaceName;
            }
        }
        ctx.state = STATE::INTERFACE_GENERIC_PARAMETERS_START;
        ctx.index--; // Re-process this character
    } else if (c == '{') {
        // Interface name complete, extract it
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string interfaceName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                if (interfaceNode->name.empty()) {
                    interfaceNode->name = interfaceName;
                }
            }
        }
        ctx.state = STATE::INTERFACE_BODY_START;
        ctx.index--; // Re-process this character
    } else if (c == 'e') {
        // Interface name complete, extract it
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string interfaceName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->name = interfaceName;
            }
        }
        ctx.state = STATE::INTERFACE_EXTENDS_E;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name, '<', 'extends', or '{'", ctx.state);
    }
}

inline void handleStateInterfaceExtendsStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // No extends, go to body
        ctx.state = STATE::INTERFACE_BODY_START;
        ctx.index--; // Re-process this character
    } else if (c == 'e') {
        ctx.state = STATE::INTERFACE_EXTENDS_E;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Unexpected character
        reportParseError(ctx.code, ctx.index, "Expected 'extends' or '{' after interface name", ctx.state);
    }
}

inline void handleStateInterfaceExtendsName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Continue building interface name
        return;
    } else if (c == ',') {
        // End of interface name, add to extends list
        std::string extendsInterface = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addExtendsInterface(extendsInterface);
        }
        ctx.state = STATE::INTERFACE_EXTENDS_SEPARATOR;
    } else if (c == '{') {
        // End of interface name, add to extends list, go to body
        std::string extendsInterface = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addExtendsInterface(extendsInterface);
        }
        ctx.state = STATE::INTERFACE_BODY_START;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name after 'extends'", ctx.state);
    }
}

inline void handleStateInterfaceExtendsSeparator(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        // Next interface name
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_EXTENDS_NAME;
    } else if (c == '{') {
        // End of extends, go to body
        ctx.state = STATE::INTERFACE_BODY_START;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected interface name or '{' after ','", ctx.state);
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
    } else if (c == '[') {
        // Index signature start
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_START;
        ctx.index--; // Re-process this character
    } else if (c == '(') {
        // Call signature start
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_START;
        ctx.index--; // Re-process this character
    } else if (isIdentifierStart(c)) {
        // Check for keywords first, then treat as property/method name
        if (c == 'n' && ctx.index + 2 <= ctx.code.length() && ctx.code.substr(ctx.index - 1, 3) == "new") {
            // 'new' keyword (construct signature)
            ctx.index += 2; // Skip 'ew'
            ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_START;
        } else if (c == 'r') {
            // 'readonly' keyword
            ctx.state = STATE::INTERFACE_READONLY_R;
        } else {
            // Property or method name
            ctx.stringStart = ctx.index - 1;
            ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        }
    } else if (std::isspace(static_cast<unsigned char>(c)) || c == ';') {
        // Skip whitespace and empty statements
    } else {
        reportParseError(ctx.code, ctx.index, "Expected property, method, index signature, call signature, construct signature, or '}' in interface body", ctx.state);
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
            // This is a property - check if it should be readonly
            auto* propNode = new InterfacePropertyNode(ctx.currentNode);
            propNode->name = propName;
            // Check if we came from readonly state
            if (ctx.state == STATE::INTERFACE_PROPERTY_READONLY) {
                propNode->isReadonly = true;
            }
            if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                interfaceNode->addInterfaceProperty(propNode);
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
    } else if (c == '?') {
        // Could be optional property or optional method
        // Look ahead to see what comes next
        std::size_t lookaheadIndex = ctx.index;
        // Skip whitespace
        while (lookaheadIndex < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[lookaheadIndex]))) {
            lookaheadIndex++;
        }
        if (lookaheadIndex < ctx.code.length()) {
            char nextChar = ctx.code[lookaheadIndex];
            if (nextChar == '(') {
                // Optional method: name?(params)
                std::string methodName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
                auto* methodNode = new ASTNode(ctx.currentNode);
                methodNode->nodeType = ASTNodeType::INTERFACE_METHOD;
                methodNode->value = methodName;
                // TODO: Mark as optional somehow
                ctx.currentNode->children.push_back(methodNode);
                ctx.currentNode = methodNode;
                ctx.state = STATE::INTERFACE_METHOD_PARAMETERS_START;
            } else if (nextChar == ':') {
                // Optional property: name?: Type
                std::string propName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
                auto* propNode = new InterfacePropertyNode(ctx.currentNode);
                propNode->name = propName;
                propNode->isOptional = true;
                // Check if we came from readonly state
                if (ctx.state == STATE::INTERFACE_PROPERTY_READONLY) {
                    propNode->isReadonly = true;
                }
                if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
                    interfaceNode->addInterfaceProperty(propNode);
                }
                ctx.currentNode = propNode;
                ctx.state = STATE::INTERFACE_PROPERTY_OPTIONAL;
            } else {
                reportParseError(ctx.code, ctx.index, "Expected '(' or ':' after '?' in interface member", ctx.state);
            }
        } else {
            reportParseError(ctx.code, ctx.index, "Unexpected end of input after '?' in interface member", ctx.state);
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue reading name
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':', '?', or '(' after property/method name", ctx.state);
    }
}

inline void handleStateInterfaceReadonlyStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after 'readonly'
        return;
    } else if (isIdentifierStart(c)) {
        // Start of property name after readonly
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
        // Mark that this property is readonly
        // We'll need to store this information somehow - for now, we'll handle it in the property creation
    } else {
        reportParseError(ctx.code, ctx.index, "Expected property name after 'readonly'", ctx.state);
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

inline void handleStateInterfacePropertyOptional(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::INTERFACE_PROPERTY_TYPE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' after optional property", ctx.state);
    }
}

inline void handleStateInterfacePropertyReadonly(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after 'readonly'
        return;
    } else if (isIdentifierStart(c)) {
        // Start of property name after readonly
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_PROPERTY_KEY;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected property name after 'readonly'", ctx.state);
    }
}



inline void handleStateInterfaceIndexSignatureStart(ParserContext& ctx, char c) {
    if (c == '[') {
        // Create index signature node
        auto* indexSigNode = new InterfaceIndexSignatureNode(ctx.currentNode);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addIndexSignature(indexSigNode);
        }
        ctx.currentNode = indexSigNode;
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_KEY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '[' to start index signature", ctx.state);
    }
}

inline void handleStateInterfaceIndexSignatureKey(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index - 1;
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_KEY_TYPE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected identifier for index signature key", ctx.state);
    }
}

inline void handleStateInterfaceIndexSignatureKeyType(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        // Continue reading key name
        return;
    } else if (c == ':') {
        // End of key name, now parse key type
        std::string keyName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* indexSigNode = dynamic_cast<InterfaceIndexSignatureNode*>(ctx.currentNode)) {
            indexSigNode->keyName = keyName;
        }
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_VALUE_TYPE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' after index signature key", ctx.state);
    }
}

inline void handleStateInterfaceIndexSignatureValueType(ParserContext& ctx, char c) {
    if (c == ']') {
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_READONLY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Type parsing would go here
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_VALUE_TYPE;
    }
}

inline void handleStateInterfaceIndexSignatureReadonly(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of index signature
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        // Type parsing would go here
        ctx.state = STATE::INTERFACE_INDEX_SIGNATURE_READONLY;
    }
}

inline void handleStateInterfaceCallSignatureStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create call signature node
        auto* callSigNode = new InterfaceCallSignatureNode(ctx.currentNode);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addCallSignature(callSigNode);
        }
        ctx.currentNode = callSigNode;
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_PARAMETERS_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '(' to start call signature", ctx.state);
    }
}

inline void handleStateInterfaceCallSignatureParametersStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_PARAMETERS_END;
    } else {
        // Parameter parsing would go here
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_PARAMETERS_START;
    }
}

inline void handleStateInterfaceCallSignatureParametersEnd(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_RETURN_TYPE;
    } else if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or ';' after call signature parameters", ctx.state);
    }
}

inline void handleStateInterfaceCallSignatureReturnType(ParserContext& ctx, char c) {
    if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else {
        // Type parsing would go here
        ctx.state = STATE::INTERFACE_CALL_SIGNATURE_RETURN_TYPE;
    }
}

inline void handleStateInterfaceConstructSignatureStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace after 'new'
        return;
    } else if (c == '(') {
        // Create construct signature node
        auto* constructSigNode = new InterfaceConstructSignatureNode(ctx.currentNode);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->addConstructSignature(constructSigNode);
        }
        ctx.currentNode = constructSigNode;
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_PARAMETERS_START;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '(' after 'new' in construct signature", ctx.state);
    }
}

inline void handleStateInterfaceConstructSignatureParametersStart(ParserContext& ctx, char c) {
    if (c == ')') {
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_PARAMETERS_END;
    } else {
        // Parameter parsing would go here
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_PARAMETERS_START;
    }
}

inline void handleStateInterfaceConstructSignatureParametersEnd(ParserContext& ctx, char c) {
    if (c == ':') {
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_RETURN_TYPE;
    } else if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else {
        reportParseError(ctx.code, ctx.index, "Expected ':' or ';' after construct signature parameters", ctx.state);
    }
}

inline void handleStateInterfaceConstructSignatureReturnType(ParserContext& ctx, char c) {
    if (c == ';') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_BODY;
    } else {
        // Type parsing would go here
        ctx.state = STATE::INTERFACE_CONSTRUCT_SIGNATURE_RETURN_TYPE;
    }
}

// Interface generic parameter parsing: interface Name<T, U>
inline void handleStateInterfaceGenericParametersStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '<') {
        // Create generic type parameters node
        auto* genericParams = new GenericTypeParametersNode(ctx.currentNode);
        if (auto* interfaceNode = dynamic_cast<InterfaceDeclarationNode*>(ctx.currentNode)) {
            interfaceNode->genericParameters = genericParams;
        }
        ctx.currentNode->children.push_back(genericParams);
        ctx.currentNode = genericParams;
        ctx.state = STATE::INTERFACE_GENERIC_PARAMETER_NAME;
    } else {
        throw std::runtime_error("Expected '<' for interface generic type parameters, got: " + std::string(1, c));
    }
}

inline void handleStateInterfaceGenericParameterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::INTERFACE_GENERIC_PARAMETER_SEPARATOR;
        return;
    }

    throw std::runtime_error("Expected identifier for interface generic type parameter, got: " + std::string(1, c));
}

inline void handleStateInterfaceGenericParameterSeparator(ParserContext& ctx, char c) {
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
        ctx.state = STATE::INTERFACE_GENERIC_PARAMETER_NAME;
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

        // Move back to parent and continue with interface declaration
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::INTERFACE_EXTENDS_START;
        return;
    }

    throw std::runtime_error("Expected ',' or '>' in interface generic type parameters, got: " + std::string(1, c));
}

inline void handleStateInterfaceGenericParametersEnd(ParserContext& ctx, char c) {
    // This state is not currently used but is included for completeness
    // The logic is handled in INTERFACE_GENERIC_PARAMETER_SEPARATOR
    throw std::runtime_error("Unexpected state: INTERFACE_GENERIC_PARAMETERS_END");
}
