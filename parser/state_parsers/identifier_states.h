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
        ctx.state = STATE::NONE;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
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
            ctx.state = STATE::NONE;
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
