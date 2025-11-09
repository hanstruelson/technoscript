#pragma once

#include <cctype>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "type_annotation_states.h"

// Conditional type parsing: T extends U ? X : Y
inline void handleStateTypeConditionalCheck(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_CONDITIONAL_E;
        return;
    }

    throw std::runtime_error("Expected type for conditional check, got: " + std::string(1, c));
}

inline void handleStateTypeConditionalE(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::TYPE_CONDITIONAL_EX;
        return;
    }
    throw std::runtime_error("Expected 'e' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalEx(ParserContext& ctx, char c) {
    if (c == 'x') {
        ctx.state = STATE::TYPE_CONDITIONAL_EXT;
        return;
    }
    throw std::runtime_error("Expected 'x' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalExt(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::TYPE_CONDITIONAL_EXTE;
        return;
    }
    throw std::runtime_error("Expected 't' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalExte(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::TYPE_CONDITIONAL_EXTEN;
        return;
    }
    throw std::runtime_error("Expected 'e' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalExten(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::TYPE_CONDITIONAL_EXTEND;
        return;
    }
    throw std::runtime_error("Expected 'n' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalExtend(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::TYPE_CONDITIONAL_EXTENDS;
        return;
    }
    throw std::runtime_error("Expected 'd' in 'extends', got: " + std::string(1, c));
}

inline void handleStateTypeConditionalExtends(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Extract the check type
        std::string checkType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart - 7); // -7 for "extends"
        // Trim trailing whitespace
        checkType.erase(checkType.find_last_not_of(" \t\n\r\f\v") + 1);

        // Create conditional type node
        auto* conditionalType = new ConditionalTypeNode(ctx.currentNode);

        // Create check type (simplified - could be any type)
        auto* checkTypeNode = new TypeAnnotationNode(conditionalType);
        if (checkType == "string") {
            checkTypeNode->dataType = DataType::STRING;
        } else {
            checkTypeNode->dataType = DataType::OBJECT; // Default
        }
        conditionalType->checkType = checkTypeNode;

        ctx.currentNode->children.push_back(conditionalType);
        ctx.currentNode = conditionalType;

        ctx.state = STATE::TYPE_CONDITIONAL_TRUE;
        return;
    }

    throw std::runtime_error("Expected space after 'extends' in conditional type, got: " + std::string(1, c));
}

inline void handleStateTypeConditionalTrue(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '?') {
        ctx.state = STATE::TYPE_CONDITIONAL_FALSE;
        return;
    }

    // Parse true type (simplified)
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_CONDITIONAL_QUESTION;
        return;
    }

    throw std::runtime_error("Expected '?' or type in conditional type, got: " + std::string(1, c));
}

inline void handleStateTypeConditionalQuestion(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '?') {
        // Extract true type
        std::string trueType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        trueType.erase(trueType.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* conditionalType = dynamic_cast<ConditionalTypeNode*>(ctx.currentNode);
        if (!conditionalType) {
            throw std::runtime_error("Expected ConditionalTypeNode");
        }

        // Create true type (simplified)
        auto* trueTypeNode = new TypeAnnotationNode(conditionalType);
        if (trueType == "string") {
            trueTypeNode->dataType = DataType::STRING;
        } else {
            trueTypeNode->dataType = DataType::OBJECT; // Default
        }
        conditionalType->trueType = trueTypeNode;

        ctx.state = STATE::TYPE_CONDITIONAL_FALSE;
        return;
    }

    throw std::runtime_error("Expected '?' in conditional type, got: " + std::string(1, c));
}

inline void handleStateTypeConditionalFalse(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION; // End of conditional type
        return;
    }

    throw std::runtime_error("Expected type for conditional false branch, got: " + std::string(1, c));
}

// Infer type parsing: infer T
inline void handleStateTypeInferI(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::TYPE_INFER_IN;
        return;
    }
    throw std::runtime_error("Expected 'i' in 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferIn(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::TYPE_INFER_INF;
        return;
    }
    throw std::runtime_error("Expected 'n' in 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferInf(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::TYPE_INFER_INFE;
        return;
    }
    throw std::runtime_error("Expected 'f' in 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferInfe(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::TYPE_INFER_INFER;
        return;
    }
    throw std::runtime_error("Expected 'e' in 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferInfer(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        ctx.state = STATE::TYPE_INFER_START;
        return;
    }
    throw std::runtime_error("Expected space after 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_INFER_NAME;
        return;
    }

    throw std::runtime_error("Expected identifier after 'infer', got: " + std::string(1, c));
}

inline void handleStateTypeInferName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION; // End of infer type
        return;
    }

    throw std::runtime_error("Expected identifier for infer type, got: " + std::string(1, c));
}

// Template literal type parsing: `Hello ${T}`
inline void handleStateTypeTemplateLiteralStart(ParserContext& ctx, char c) {
    if (c == '`') {
        auto* templateType = new TemplateLiteralTypeNode(ctx.currentNode);
        ctx.currentNode->children.push_back(templateType);
        ctx.currentNode = templateType;
        ctx.state = STATE::TYPE_TEMPLATE_LITERAL_QUASI;
        return;
    }

    throw std::runtime_error("Expected '`' for template literal type, got: " + std::string(1, c));
}

inline void handleStateTypeTemplateLiteralQuasi(ParserContext& ctx, char c) {
    if (c == '$' && ctx.code[ctx.index + 1] == '{') {
        // End of quasi, start interpolation
        std::string quasi = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* templateType = dynamic_cast<TemplateLiteralTypeNode*>(ctx.currentNode);
        if (templateType) {
            templateType->addQuasi(quasi);
        }
        ctx.index++; // Skip '{'
        ctx.state = STATE::TYPE_TEMPLATE_LITERAL_INTERPOLATION;
        return;
    }

    if (c == '`') {
        // End of template literal
        std::string quasi = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        auto* templateType = dynamic_cast<TemplateLiteralTypeNode*>(ctx.currentNode);
        if (templateType) {
            templateType->addQuasi(quasi);
        }
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::TYPE_ANNOTATION;
        return;
    }

    // Continue with quasi
    if (ctx.stringStart == 0) {
        ctx.stringStart = ctx.index;
    }
}

inline void handleStateTypeTemplateLiteralInterpolation(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of interpolation, back to quasi
        ctx.state = STATE::TYPE_TEMPLATE_LITERAL_QUASI;
        ctx.stringStart = 0; // Reset for next quasi
        return;
    }

    // Parse type in interpolation (simplified)
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        if (ctx.stringStart == 0) {
            ctx.stringStart = ctx.index;
        }
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    throw std::runtime_error("Unexpected character in template literal interpolation: " + std::string(1, c));
}

// Mapped type parsing: {[K in T]: V}
inline void handleStateTypeMappedStart(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '{') {
        auto* mappedType = new MappedTypeNode(ctx.currentNode);
        ctx.currentNode->children.push_back(mappedType);
        ctx.currentNode = mappedType;
        ctx.state = STATE::TYPE_MAPPED_R;
        return;
    }

    throw std::runtime_error("Expected '{' for mapped type, got: " + std::string(1, c));
}

inline void handleStateTypeMappedR(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::TYPE_MAPPED_RE;
        return;
    }
    // No readonly, continue to parameter
    ctx.state = STATE::TYPE_MAPPED_PARAMETER;
    ctx.index--; // Re-process this character
}

inline void handleStateTypeMappedRe(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::TYPE_MAPPED_REA;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 2;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedRea(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::TYPE_MAPPED_READ;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 3;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedRead(ParserContext& ctx, char c) {
    if (c == 'd') {
        ctx.state = STATE::TYPE_MAPPED_READO;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 4;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedReado(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::TYPE_MAPPED_READON;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 5;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedReadon(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::TYPE_MAPPED_READONL;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 6;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedReadonl(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::TYPE_MAPPED_READONLY;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 7;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedReadonly(ParserContext& ctx, char c) {
    if (c == 'y') {
        auto* mappedType = dynamic_cast<MappedTypeNode*>(ctx.currentNode);
        if (mappedType) {
            mappedType->isReadonly = true;
        }
        ctx.state = STATE::TYPE_MAPPED_PARAMETER;
        return;
    }
    // Not readonly, treat as type identifier
    ctx.stringStart = ctx.index - 8;
    ctx.state = STATE::TYPE_ANNOTATION;
}

inline void handleStateTypeMappedParameter(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '[') {
        ctx.state = STATE::TYPE_MAPPED_IN;
        return;
    }

    throw std::runtime_error("Expected '[' for mapped type parameter, got: " + std::string(1, c));
}

inline void handleStateTypeMappedI(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::TYPE_MAPPED_IN;
        return;
    }
    throw std::runtime_error("Expected 'i' in 'in', got: " + std::string(1, c));
}

inline void handleStateTypeMappedIn(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::TYPE_MAPPED_IN_CONSTRAINT;
        return;
    }
    throw std::runtime_error("Expected 'n' in 'in', got: " + std::string(1, c));
}

inline void handleStateTypeMappedInConstraint(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Extract parameter name
        std::string paramName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart - 2); // -2 for "in"
        // Trim trailing whitespace
        paramName.erase(paramName.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* mappedType = dynamic_cast<MappedTypeNode*>(ctx.currentNode);
        if (mappedType) {
            mappedType->typeParameter = paramName;
        }

        ctx.state = STATE::TYPE_MAPPED_CONSTRAINT;
        return;
    }
    throw std::runtime_error("Expected space after 'in' in mapped type, got: " + std::string(1, c));
}

inline void handleStateTypeMappedConstraint(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == 'a') {
        ctx.state = STATE::TYPE_MAPPED_A;
        return;
    }

    if (c == ']') {
        // Extract constraint type
        std::string constraintType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        constraintType.erase(constraintType.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* mappedType = dynamic_cast<MappedTypeNode*>(ctx.currentNode);
        if (mappedType) {
            // Create constraint type (simplified)
            auto* constraintTypeNode = new TypeAnnotationNode(mappedType);
            if (constraintType == "string") {
                constraintTypeNode->dataType = DataType::STRING;
            } else {
                constraintTypeNode->dataType = DataType::OBJECT; // Default
            }
            mappedType->constraintType = constraintTypeNode;
        }

        ctx.state = STATE::TYPE_MAPPED_OPTIONAL;
        return;
    }

    throw std::runtime_error("Expected 'as' or ']' in mapped type constraint, got: " + std::string(1, c));
}

inline void handleStateTypeMappedA(ParserContext& ctx, char c) {
    if (c == 'a') {
        ctx.state = STATE::TYPE_MAPPED_AS;
        return;
    }
    // Not "as", treat as constraint type
    ctx.stringStart = ctx.index - 1; // Include the 'a' we just consumed
    ctx.state = STATE::TYPE_MAPPED_VALUE;
    return;
}

inline void handleStateTypeMappedAs(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::TYPE_MAPPED_AS_NAME;
        return;
    }
    // Not "as", treat as constraint type starting with 'a'
    ctx.stringStart = ctx.index - 2; // Include "a" and current char
    ctx.state = STATE::TYPE_MAPPED_VALUE;
    return;
}

inline void handleStateTypeMappedAsName(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_MAPPED_VALUE;
        return;
    }

    throw std::runtime_error("Expected name after 'as' in mapped type, got: " + std::string(1, c));
}

inline void handleStateTypeMappedValue(ParserContext& ctx, char c) {
    if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
        return;
    }

    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == ']') {
        // Extract constraint type
        std::string constraintType = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        // Trim trailing whitespace
        constraintType.erase(constraintType.find_last_not_of(" \t\n\r\f\v") + 1);

        auto* mappedType = dynamic_cast<MappedTypeNode*>(ctx.currentNode);
        if (mappedType) {
            // Create constraint type (simplified)
            auto* constraintTypeNode = new TypeAnnotationNode(mappedType);
            if (constraintType == "string") {
                constraintTypeNode->dataType = DataType::STRING;
            } else {
                constraintTypeNode->dataType = DataType::OBJECT; // Default
            }
            mappedType->constraintType = constraintTypeNode;
        }

        ctx.state = STATE::TYPE_MAPPED_OPTIONAL;
        return;
    }

    throw std::runtime_error("Expected ']' in mapped type, got: " + std::string(1, c));
}

inline void handleStateTypeMappedOptional(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    if (c == '?') {
        auto* mappedType = dynamic_cast<MappedTypeNode*>(ctx.currentNode);
        if (mappedType) {
            mappedType->isOptional = true;
        }
        ctx.state = STATE::TYPE_ANNOTATION; // End of mapped type
        return;
    }

    // No optional, continue to value type
    ctx.state = STATE::TYPE_ANNOTATION;
    ctx.index--; // Re-process this character
}
