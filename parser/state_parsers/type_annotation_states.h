#pragma once

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "generic_states.h"

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
    } else if (c == '>') {
        // This might be a leftover '>' from generic type parsing
        // Just ignore it for now
        return;
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

    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    }

    if (c == '<') {
        // Generic type usage: Array<T>, Promise<T, U>
        ctx.state = STATE::TYPE_GENERIC_TYPE_START;
        ctx.index--;
        return;
    }

    if (c == '>') {
        // This might be the end of a generic type that we just parsed
        // Continue to EXPECT_EQUALS state
        ctx.state = STATE::EXPECT_EQUALS;
        ctx.index--;
        return;
    }

    if (c == '|') {
        // Union type - create union type node if not already created
        std::string currentType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        currentType.erase(currentType.find_last_not_of(" \t\n\r\f\v") + 1);

        // Find the variable definition node
        ASTNode* current = ctx.currentNode;
        while (current && current->nodeType != ASTNodeType::VARIABLE_DEFINITION) {
            current = current->parent;
        }
        auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(current);
        if (!varDefNode) {
            throw std::runtime_error("Current context is not a VariableDefinitionNode for union type");
        }

        // Create union type node if this is the first |
        if (!varDefNode->typeAnnotation || varDefNode->typeAnnotation->nodeType != ASTNodeType::UNION_TYPE) {
            auto* unionType = new UnionTypeNode(varDefNode);
            // If there's already a simple type, move it to the union
            if (varDefNode->typeAnnotation) {
                unionType->addType(varDefNode->typeAnnotation);
                varDefNode->children.clear(); // Remove from children, will be re-added
            }
            varDefNode->typeAnnotation = unionType;
            varDefNode->children.push_back(unionType);
        }

        // Add current type to union
        auto* unionType = dynamic_cast<UnionTypeNode*>(varDefNode->typeAnnotation);
        auto* typeNode = new TypeAnnotationNode(unionType);
        if (currentType == "int64") {
            typeNode->dataType = DataType::INT64;
        } else {
            throw std::runtime_error("Unknown type annotation: " + currentType);
        }
        unionType->addType(typeNode);

        // Continue parsing next type in union
        ctx.state = STATE::TYPE_ANNOTATION;
        // Skip whitespace after | and set stringStart for next type
        std::size_t nextStart = ctx.index + 1;
        while (nextStart < ctx.code.length() && std::isspace(static_cast<unsigned char>(ctx.code[nextStart]))) {
            nextStart++;
        }
        ctx.stringStart = nextStart;
        return;
    }

    // End of type annotation (encountered = or other terminator)
    std::string typeAnnotation = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    // Trim trailing whitespace
    typeAnnotation.erase(typeAnnotation.find_last_not_of(" \t\n\r\f\v") + 1);

    // Find the variable definition node
    ASTNode* current = ctx.currentNode;
    while (current && current->nodeType != ASTNodeType::VARIABLE_DEFINITION) {
        current = current->parent;
    }
    auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(current);
    if (!varDefNode) {
        throw std::runtime_error("Invalid context for type annotation");
    }

    if (!varDefNode->typeAnnotation) {
        // Simple type annotation
        auto* typeNode = new TypeAnnotationNode(varDefNode);
        if (typeAnnotation == "int64") {
            typeNode->dataType = DataType::INT64;
        } else {
            throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
        }
        varDefNode->typeAnnotation = typeNode;
        varDefNode->children.push_back(typeNode);
    } else if (auto* unionType = dynamic_cast<UnionTypeNode*>(varDefNode->typeAnnotation)) {
        // Adding the last type to union
        auto* typeNode = new TypeAnnotationNode(unionType);
        if (typeAnnotation == "int64") {
            typeNode->dataType = DataType::INT64;
        } else {
            throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
        }
        unionType->addType(typeNode);
    }

    ctx.state = STATE::EXPECT_EQUALS;
    ctx.index--;
}
