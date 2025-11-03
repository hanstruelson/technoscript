#pragma once

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "type_annotation_states.h"

// Generic type parameter parsing: <T, U, V>
inline void handleStateTypeGenericParametersStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '<') {
        // Create generic type parameters node
        auto* genericParams = new GenericTypeParametersNode(ctx.currentNode);
        ctx.currentNode->children.push_back(genericParams);
        ctx.currentNode = genericParams;
        ctx.state = STATE::TYPE_GENERIC_PARAMETER_NAME;
    } else {
        throw std::runtime_error("Expected '<' for generic type parameters, got: " + std::string(1, c));
    }
}

inline void handleStateTypeGenericParameterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_GENERIC_PARAMETER_SEPARATOR;
        return;
    }

    throw std::runtime_error("Expected identifier for generic type parameter, got: " + std::string(1, c));
}

inline void handleStateTypeGenericParameterSeparator(ParserContext& ctx, char c) {
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
        ctx.state = STATE::TYPE_GENERIC_PARAMETER_NAME;
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

        // Move back to parent and continue with type annotation
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::TYPE_ANNOTATION;
        return;
    }

    throw std::runtime_error("Expected ',' or '>' in generic type parameters, got: " + std::string(1, c));
}

inline void handleStateTypeGenericParametersEnd(ParserContext& ctx, char c) {
    // This state is not currently used but is included for completeness
    // The logic is handled in TYPE_GENERIC_PARAMETER_SEPARATOR
    throw std::runtime_error("Unexpected state: TYPE_GENERIC_PARAMETERS_END");
}

// Generic type usage parsing: Array<T>, Promise<T, U>
inline void handleStateTypeGenericTypeStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '<') {
        // Get the base type name from before the <
        std::string baseType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        baseType.erase(baseType.find_last_not_of(" \t\n\r\f\v") + 1);

        // Create generic type node
        auto* genericType = new GenericTypeNode(ctx.currentNode);
        genericType->baseType = baseType;
        ctx.currentNode->children.push_back(genericType);
        ctx.currentNode = genericType;

        ctx.state = STATE::TYPE_GENERIC_TYPE_ARGUMENTS;
    } else {
        throw std::runtime_error("Expected '<' for generic type arguments, got: " + std::string(1, c));
    }
}

inline void handleStateTypeGenericTypeArguments(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION;
        return;
    }

    if (c == ',') {
        // Add previous type argument and continue
        // For now, we'll handle simple type names - this could be extended
        std::string typeArg = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        typeArg.erase(typeArg.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* genericType = dynamic_cast<GenericTypeNode*>(ctx.currentNode);
        if (!genericType) {
            throw std::runtime_error("Expected GenericTypeNode");
        }

        // Create a simple type annotation for the argument
        auto* typeNode = new TypeAnnotationNode(genericType);
        if (typeArg == "int64") {
            typeNode->dataType = DataType::INT64;
        } else if (typeArg == "float64") {
            typeNode->dataType = DataType::FLOAT64;
        } else if (typeArg == "string") {
            typeNode->dataType = DataType::STRING;
        } else {
            // For unknown types, default to object
            typeNode->dataType = DataType::OBJECT;
        }
        genericType->addTypeArgument(typeNode);

        ctx.state = STATE::TYPE_GENERIC_TYPE_ARGUMENTS;
        return;
    }

    if (c == '>') {
        // Add the last type argument
        std::string typeArg = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        typeArg.erase(typeArg.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* genericType = dynamic_cast<GenericTypeNode*>(ctx.currentNode);
        if (!genericType) {
            throw std::runtime_error("Expected GenericTypeNode");
        }

        // Create a simple type annotation for the argument
        auto* typeNode = new TypeAnnotationNode(genericType);
        if (typeArg == "int64") {
            typeNode->dataType = DataType::INT64;
        } else if (typeArg == "float64") {
            typeNode->dataType = DataType::FLOAT64;
        } else if (typeArg == "string") {
            typeNode->dataType = DataType::STRING;
        } else {
            // For unknown types, default to object
            typeNode->dataType = DataType::OBJECT;
        }
        genericType->addTypeArgument(typeNode);

        // Move back to parent - the generic type is now complete
        ctx.currentNode = ctx.currentNode->parent;

        // Set the type annotation on the variable definition
        ASTNode* current = ctx.currentNode;
        while (current && current->nodeType != ASTNodeType::VARIABLE_DEFINITION) {
            current = current->parent;
        }
        auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(current);
        if (varDefNode) {
            varDefNode->typeAnnotation = genericType;
            varDefNode->children.push_back(genericType);
        }

        // We've consumed the '>' character, now set state for what comes next
        ctx.state = STATE::EXPECT_EQUALS;
        return;
    }

    throw std::runtime_error("Unexpected character in generic type arguments: " + std::string(1, c));
}
