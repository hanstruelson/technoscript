#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"
#include "../parser.h"
#include "../state.h"

// Type alias parsing states
inline void handleStateBlockTY(ParserContext& ctx, char c) {
    if (c == 'p') {
        ctx.state = STATE::BLOCK_TYP;
    } else {
        throw std::runtime_error("Expected 'p' after 'ty': " + std::string(1, c));
    }
}

inline void handleStateBlockTYP(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::BLOCK_TYPE;
    } else {
        throw std::runtime_error("Expected 'e' after 'typ': " + std::string(1, c));
    }
}

inline void handleStateBlockTYPE(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Create type alias node
        auto* typeAliasNode = new TypeAliasNode(ctx.currentNode);
        ctx.currentNode->children.push_back(typeAliasNode);
        ctx.currentNode = typeAliasNode;
        ctx.stringStart = 0; // Reset string parsing state
        ctx.state = STATE::TYPE_ALIAS_NAME;
    } else {
        throw std::runtime_error("Expected space after 'type': " + std::string(1, c));
    }
}

inline void handleStateTypeAliasName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Check if we've started parsing the type alias name
        if (ctx.stringStart > 0) {
            // Type alias name complete, extract it
            std::string typeAliasName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* typeAliasNode = dynamic_cast<TypeAliasNode*>(ctx.currentNode)) {
                typeAliasNode->name = typeAliasName;
            }
            ctx.state = STATE::TYPE_ALIAS_EQUALS;
        } else {
            // Skip whitespace before type alias name starts
            return;
        }
    } else if (ctx.stringStart == 0 && isIdentifierStart(c)) {
        // Start of type alias name
        ctx.stringStart = ctx.index - 1;
        // Stay in this state to continue parsing
    } else if (isIdentifierPart(c)) {
        // Continue parsing type alias name - accumulate characters
        return;
    } else if (c == '<') {
        // Generic parameters start
        if (ctx.stringStart > 0 && ctx.stringStart < ctx.index) {
            std::string typeAliasName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
            if (auto* typeAliasNode = dynamic_cast<TypeAliasNode*>(ctx.currentNode)) {
                typeAliasNode->name = typeAliasName;
            }
        }
        ctx.state = STATE::TYPE_ALIAS_GENERIC_PARAMETERS_START;
        ctx.index--; // Re-process this character
    } else {
        reportParseError(ctx.code, ctx.index, "Expected type alias name or '<'", ctx.state);
    }
}

inline void handleStateTypeAliasGenericParametersStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '<') {
        // Create generic type parameters node
        auto* genericParams = new GenericTypeParametersNode(ctx.currentNode);
        if (auto* typeAliasNode = dynamic_cast<TypeAliasNode*>(ctx.currentNode)) {
            typeAliasNode->genericParameters = genericParams;
        }
        ctx.currentNode->children.push_back(genericParams);
        ctx.currentNode = genericParams;
        ctx.state = STATE::TYPE_ALIAS_GENERIC_PARAMETER_NAME;
    } else {
        throw std::runtime_error("Expected '<' for type alias generic type parameters, got: " + std::string(1, c));
    }
}

inline void handleStateTypeAliasGenericParameterName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ALIAS_GENERIC_PARAMETER_SEPARATOR;
        return;
    }

    throw std::runtime_error("Expected identifier for type alias generic type parameter, got: " + std::string(1, c));
}

inline void handleStateTypeAliasGenericParameterSeparator(ParserContext& ctx, char c) {
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
        ctx.state = STATE::TYPE_ALIAS_GENERIC_PARAMETER_NAME;
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

        // Move back to parent and continue with type alias
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::TYPE_ALIAS_EQUALS;
        return;
    }

    throw std::runtime_error("Expected ',' or '>' in type alias generic type parameters, got: " + std::string(1, c));
}

inline void handleStateTypeAliasGenericParametersEnd(ParserContext& ctx, char c) {
    // This state is not currently used but is included for completeness
    // The logic is handled in TYPE_ALIAS_GENERIC_PARAMETER_SEPARATOR
    throw std::runtime_error("Unexpected state: TYPE_ALIAS_GENERIC_PARAMETERS_END");
}

inline void handleStateTypeAliasEquals(ParserContext& ctx, char c) {
    if (c == '=') {
        ctx.state = STATE::TYPE_ALIAS_TYPE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        reportParseError(ctx.code, ctx.index, "Expected '=' after type alias name", ctx.state);
    }
}

inline void handleStateTypeAliasType(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of type alias
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::BLOCK;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip type parsing - complex type parsing needed
        ctx.state = STATE::TYPE_ALIAS_TYPE;
    }
}
