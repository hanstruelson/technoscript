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

inline void handleStateTypeAnnotationWhitespace(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue consuming whitespace
        return;
    }

    // Non-whitespace character encountered, go back to TYPE_ANNOTATION
    ctx.index--; // Re-process this character
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeUnionSeparatorWhitespace(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue consuming whitespace
        return;
    }

    // Non-whitespace character encountered, start next type in union
    ctx.index--; // Re-process this character
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeIntersectionSeparatorWhitespace(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Continue consuming whitespace
        return;
    }

    // Non-whitespace character encountered, start next type in intersection
    ctx.index--; // Re-process this character
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeAnnotation(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        // Transition to whitespace state instead of skipping
        ctx.state = STATE::TYPE_ANNOTATION_WHITESPACE;
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

    // Check for advanced generics keywords
    std::string currentToken = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);

    if (c == 'i') {
        // Infer type
        ctx.state = STATE::TYPE_INFER_I;
        return;
    }

    if (c == '`') {
        // Template literal type
        ctx.state = STATE::TYPE_TEMPLATE_LITERAL_START;
        return;
    }



    // Check for conditional type (T extends U ? X : Y)
    if (c == 'e') {
        // Start conditional type parsing
        ctx.state = STATE::TYPE_CONDITIONAL_CHECK;
        return;
    }

    if (c == '|') {
        // Union type - create union type node if not already created
        std::string currentType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);

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
        if (currentType == "int64" || currentType == "int") {
            typeNode->dataType = DataType::INT64;
        } else {
            throw std::runtime_error("Unknown type annotation: " + currentType);
        }
        unionType->addType(typeNode);

        // Continue parsing next type in union after whitespace
        ctx.state = STATE::TYPE_UNION_SEPARATOR_WHITESPACE;
        return;
    }

    if (c == '&') {
        // Intersection type - create intersection type node if not already created
        std::string currentType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);

        // Find the variable definition node
        ASTNode* current = ctx.currentNode;
        while (current && current->nodeType != ASTNodeType::VARIABLE_DEFINITION) {
            current = current->parent;
        }
        auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(current);
        if (!varDefNode) {
            throw std::runtime_error("Current context is not a VariableDefinitionNode for intersection type");
        }

        // Create intersection type node if this is the first &
        if (!varDefNode->typeAnnotation || varDefNode->typeAnnotation->nodeType != ASTNodeType::INTERSECTION_TYPE) {
            auto* intersectionType = new IntersectionTypeNode(varDefNode);
            // If there's already a simple type, move it to the intersection
            if (varDefNode->typeAnnotation) {
                intersectionType->addType(varDefNode->typeAnnotation);
                varDefNode->children.clear(); // Remove from children, will be re-added
            }
            varDefNode->typeAnnotation = intersectionType;
            varDefNode->children.push_back(intersectionType);
        }

        // Add current type to intersection
        auto* intersectionType = dynamic_cast<IntersectionTypeNode*>(varDefNode->typeAnnotation);
        auto* typeNode = new TypeAnnotationNode(intersectionType);
        if (currentType == "int64" || currentType == "int") {
            typeNode->dataType = DataType::INT64;
        } else {
            throw std::runtime_error("Unknown type annotation: " + currentType);
        }
        intersectionType->addType(typeNode);

        // Continue parsing next type in intersection after whitespace
        ctx.state = STATE::TYPE_INTERSECTION_SEPARATOR_WHITESPACE;
        return;
    }

    // End of type annotation (encountered = or other terminator)
    std::string typeAnnotation = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
    // Trim surrounding whitespace to avoid trailing spaces from annotations like ": int64"
    auto firstNonSpace = typeAnnotation.find_first_not_of(" \t\n\r\f\v");
    if (firstNonSpace == std::string::npos) {
        throw std::runtime_error("Empty type annotation");
    }
    auto lastNonSpace = typeAnnotation.find_last_not_of(" \t\n\r\f\v");
    typeAnnotation = typeAnnotation.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);

    // Find the appropriate node (variable definition or function)
    ASTNode* current = ctx.currentNode;
    while (current && current->nodeType != ASTNodeType::VARIABLE_DEFINITION &&
           current->nodeType != ASTNodeType::FUNCTION_DECLARATION &&
           current->nodeType != ASTNodeType::FUNCTION_EXPRESSION &&
           current->nodeType != ASTNodeType::ARROW_FUNCTION_EXPRESSION) {
        current = current->parent;
    }

    if (auto* varDefNode = dynamic_cast<VariableDefinitionNode*>(current)) {
        // Handle variable type annotation
        if (!varDefNode->typeAnnotation) {
            // Simple type annotation
            auto* typeNode = new TypeAnnotationNode(varDefNode);
            if (typeAnnotation == "int64" || typeAnnotation == "int") {
                typeNode->dataType = DataType::INT64;
            } else {
                throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
            }
            varDefNode->typeAnnotation = typeNode;
            varDefNode->children.push_back(typeNode);
        } else if (auto* unionType = dynamic_cast<UnionTypeNode*>(varDefNode->typeAnnotation)) {
            // Adding the last type to union
            auto* typeNode = new TypeAnnotationNode(unionType);
            if (typeAnnotation == "int64" || typeAnnotation == "int") {
                typeNode->dataType = DataType::INT64;
            } else {
                throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
            }
            unionType->addType(typeNode);
        } else if (auto* intersectionType = dynamic_cast<IntersectionTypeNode*>(varDefNode->typeAnnotation)) {
            // Adding the last type to intersection
            auto* typeNode = new TypeAnnotationNode(intersectionType);
            if (typeAnnotation == "int64" || typeAnnotation == "int") {
                typeNode->dataType = DataType::INT64;
            } else {
                throw std::runtime_error("Unknown type annotation: " + typeAnnotation);
            }
            intersectionType->addType(typeNode);
        }

        // Call the callback when type annotation is complete
        varDefNode->onTypeAnnotationComplete(ctx);

        ctx.state = STATE::EXPECT_EQUALS;
    } else if (auto* funcDeclNode = dynamic_cast<FunctionDeclarationNode*>(current)) {
        // Handle function return type annotation
        if (!funcDeclNode->returnType) {
            auto* typeNode = new TypeAnnotationNode(funcDeclNode);
            if (typeAnnotation == "int64") {
                typeNode->dataType = DataType::INT64;
            } else if (typeAnnotation == "string") {
                typeNode->dataType = DataType::STRING;
            } else if (typeAnnotation == "float64") {
                typeNode->dataType = DataType::FLOAT64;
            } else {
                // Default to object for unknown types
                typeNode->dataType = DataType::OBJECT;
            }
            funcDeclNode->returnType = typeNode;
            funcDeclNode->children.push_back(typeNode);
        }

        // Call the callback when type annotation is complete
        funcDeclNode->onTypeAnnotationComplete(ctx);

        ctx.state = STATE::FUNCTION_BODY_START;
    } else if (auto* funcExprNode = dynamic_cast<FunctionExpressionNode*>(current)) {
        // Handle function expression return type annotation
        if (!funcExprNode->returnType) {
            auto* typeNode = new TypeAnnotationNode(funcExprNode);
            if (typeAnnotation == "int64") {
                typeNode->dataType = DataType::INT64;
            } else if (typeAnnotation == "string") {
                typeNode->dataType = DataType::STRING;
            } else if (typeAnnotation == "float64") {
                typeNode->dataType = DataType::FLOAT64;
            } else {
                // Default to object for unknown types
                typeNode->dataType = DataType::OBJECT;
            }
            funcExprNode->returnType = typeNode;
            funcExprNode->children.push_back(typeNode);
        }

        // Call the callback when type annotation is complete
        funcExprNode->onTypeAnnotationComplete(ctx);

        ctx.state = STATE::FUNCTION_BODY_START;
    } else if (auto* arrowFuncNode = dynamic_cast<ArrowFunctionExpressionNode*>(current)) {
        // Handle arrow function return type annotation
        if (!arrowFuncNode->returnType) {
            auto* typeNode = new TypeAnnotationNode(arrowFuncNode);
            if (typeAnnotation == "int64") {
                typeNode->dataType = DataType::INT64;
            } else if (typeAnnotation == "string") {
                typeNode->dataType = DataType::STRING;
            } else if (typeAnnotation == "float64") {
                typeNode->dataType = DataType::FLOAT64;
            } else {
                // Default to object for unknown types
                typeNode->dataType = DataType::OBJECT;
            }
            arrowFuncNode->returnType = typeNode;
            arrowFuncNode->children.push_back(typeNode);
        }

        // Call the callback when type annotation is complete
        arrowFuncNode->onTypeAnnotationComplete(ctx);

        ctx.state = STATE::ARROW_FUNCTION_BODY;
    } else {
        throw std::runtime_error("Invalid context for type annotation");
    }

    ctx.index--;
}
