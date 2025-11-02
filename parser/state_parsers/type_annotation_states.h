#pragma once

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Define in order of dependency - handleStateExpectEquals first
inline void handleStateExpectEquals(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '=') {
        auto* newNode = new ExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(newNode);
        ctx.currentNode = newNode;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateExpectTypeAnnotation(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        
    } else if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION;
    } else {
        std::cout << "doing await type annotation";
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}

inline void handleStateTypeAnnotation(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    std::string typeAnnotation = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(ctx.currentNode);
    if (!varDefNode) {
        throw std::runtime_error("Current node is not a VariableDefinitionNode");
    }
    varDefNode->typeAnnotation = new TypeAnnotationNode(varDefNode);
    varDefNode->children.push_back(varDefNode->typeAnnotation);
    if (typeAnnotation == "int64") {
        varDefNode->typeAnnotation->dataType = DataType::INT64;
    } else {
        throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
    }
    ctx.state = STATE::EXPECT_EQUALS;
    handleStateExpectEquals(ctx, c);
}
