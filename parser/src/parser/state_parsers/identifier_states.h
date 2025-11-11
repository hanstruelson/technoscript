#pragma once

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"

// Define in order of dependency - handleStateVariableCreateIdentifierComplete first
inline void handleStateVariableCreateIdentifierComplete(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {

    } else if (c == ':') {
        ctx.state = STATE::EXPECT_TYPE_ANNOTATION;
    } else if (c == '=') {
        auto* varNode = dynamic_cast<VariableDefinitionNode*>(ctx.currentNode);
        auto* newNode = new ExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(newNode);
        if (varNode) {
            varNode->initializer = newNode;
        }
        ctx.currentNode = newNode;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == ';') {
        if (ctx.currentNode && ctx.currentNode->parent) {
            ctx.currentNode = ctx.currentNode->parent;
        }
        ctx.state = STATE::BLOCK;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateFunctionParameterComplete(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (c == ':') {
        // Type annotation
        ctx.state = STATE::FUNCTION_PARAMETER_TYPE_ANNOTATION;
    } else if (c == '=') {
        // Default value
        ctx.state = STATE::FUNCTION_PARAMETER_DEFAULT_VALUE;
    } else if (c == ',' || c == ')') {
        // Parameter complete, move back to parameter list
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
    } else {
        throw std::runtime_error("Unexpected character in function parameter: " + std::string(1, c));
    }
}

inline void handleStateIdentifierComplete(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else if (c == ':') {
        std::cout << "C is " << c << endl;
        ctx.state = STATE::EXPECT_TYPE_ANNOTATION;
    } else if (c == '=') {
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == ';') {
        auto* parent = ctx.currentNode->parent;
        if (parent) {
            parent->children.push_back(ctx.currentNode);
            ctx.currentNode = parent;
            ctx.state = STATE::BLOCK;
        } else {
            throw std::runtime_error("Identifier completion with no parent node");
        }
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateExpectIdentifier(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::IDENTIFIER_NAME;
    } else if (c == '[') {
        // Array destructuring
        auto* arrayNode = new ArrayDestructuringNode(ctx.currentNode);
        if (auto* varNode = dynamic_cast<VariableDefinitionNode*>(ctx.currentNode)) {
            varNode->pattern = arrayNode;
        }
        ctx.currentNode = arrayNode;
        ctx.state = STATE::ARRAY_DESTRUCTURING_START;
    } else if (c == '{') {
        // Object destructuring
        auto* objectNode = new ObjectDestructuringNode(ctx.currentNode);
        if (auto* varNode = dynamic_cast<VariableDefinitionNode*>(ctx.currentNode)) {
            varNode->pattern = objectNode;
        }
        ctx.currentNode = objectNode;
        ctx.state = STATE::OBJECT_DESTRUCTURING_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateIdentifierName(ParserContext& ctx, char c) {
    if (isIdentifierPart(c)) {
        return;
    }

    std::string identifier = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    if (ctx.currentNode->nodeType == ASTNodeType::VARIABLE_DEFINITION) {
        auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(ctx.currentNode);
        varDefNode->name = identifier;

        // Add variable to appropriate scope
        if (!identifier.empty()) {
            VariableInfo varInfo;
            varInfo.name = identifier;
            varInfo.varType = varDefNode->varType;
            varInfo.type = DataType::INT64;
            varInfo.size = 8;

            if (varDefNode->varType == VariableDefinitionType::VAR) {
                // Add to function scope
                if (ctx.currentFunctionScope) {
                    varInfo.definingScope = ctx.currentFunctionScope;
                    ctx.currentFunctionScope->variables[identifier] = varInfo;
                }
            } else {
                // Add to block scope (let/const)
                if (ctx.currentBlockScope) {
                    varInfo.definingScope = ctx.currentBlockScope;
                    ctx.currentBlockScope->variables[identifier] = varInfo;
                }
            }
        }

        ctx.state = STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE;
        ctx.index--;
    } else {
        ctx.state = STATE::IDENTIFIER_COMPLETE;
        ctx.index--;
    }
}

inline void handleStateExpectImmediateIdentifier(ParserContext& ctx, char c) {
    if (isIdentifierStart(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::IDENTIFIER_NAME;
    } else {
        throw std::runtime_error("Expected identifier start character, got: " + std::string(1, c));
    }
}
