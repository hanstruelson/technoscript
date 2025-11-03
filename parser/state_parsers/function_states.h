#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Forward declarations for function state handlers
inline void handleStateFunctionDeclarationName(ParserContext& ctx, char c);
inline void handleStateFunctionParametersStart(ParserContext& ctx, char c);
inline void handleStateFunctionParameterName(ParserContext& ctx, char c);
inline void handleStateFunctionParameterTypeAnnotation(ParserContext& ctx, char c);
inline void handleStateFunctionParameterDefaultValue(ParserContext& ctx, char c);
inline void handleStateFunctionParameterSeparator(ParserContext& ctx, char c);
inline void handleStateFunctionParametersEnd(ParserContext& ctx, char c);
inline void handleStateFunctionReturnTypeAnnotation(ParserContext& ctx, char c);
inline void handleStateFunctionBodyStart(ParserContext& ctx, char c);
inline void handleStateFunctionBody(ParserContext& ctx, char c);
inline void handleStateArrowFunctionParameters(ParserContext& ctx, char c);
inline void handleStateArrowFunctionArrow(ParserContext& ctx, char c);
inline void handleStateArrowFunctionBody(ParserContext& ctx, char c);

// Function keyword detection states
inline void handleStateNoneF(ParserContext& ctx, char c) {
    if (c == 'u') {
        ctx.state = STATE::NONE_FU;
    } else {
        throw std::runtime_error("Unexpected character after 'f': " + std::string(1, c));
    }
}

inline void handleStateNoneFU(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::NONE_FUN;
    } else {
        throw std::runtime_error("Unexpected character after 'fu': " + std::string(1, c));
    }
}

inline void handleStateNoneFUN(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::NONE_FUNC;
    } else {
        throw std::runtime_error("Unexpected character after 'fun': " + std::string(1, c));
    }
}

inline void handleStateNoneFUNC(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_FUNCT;
    } else {
        throw std::runtime_error("Unexpected character after 'func': " + std::string(1, c));
    }
}

inline void handleStateNoneFUNCT(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_FUNCTI;
    } else {
        throw std::runtime_error("Unexpected character after 'funct': " + std::string(1, c));
    }
}

inline void handleStateNoneFUNCTI(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_FUNCTIO;
    } else {
        throw std::runtime_error("Unexpected character after 'functi': " + std::string(1, c));
    }
}

inline void handleStateNoneFUNCTIO(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::NONE_FUNCTION;
    } else {
        throw std::runtime_error("Unexpected character after 'functio': " + std::string(1, c));
    }
}

inline void handleStateNoneFUNCTION(ParserContext& ctx, char c) {
    if (c == ' ') {
        // Function declaration detected, create function node
        auto* funcNode = new FunctionDeclarationNode(ctx.currentNode);
        ctx.currentNode->children.push_back(funcNode);
        ctx.currentNode = funcNode;
        ctx.state = STATE::FUNCTION_DECLARATION_NAME;
    } else {
        throw std::runtime_error("Expected space after 'function': " + std::string(1, c));
    }
}

// Function declaration states
inline void handleStateFunctionDeclarationName(ParserContext& ctx, char c) {
    if (isalpha(c) || c == '_') {
        // Start of function name
        ctx.stringStart = ctx.index;
        // Stay in this state to continue parsing
    } else if (isalnum(c) || c == '_') {
        // Continue parsing function name - accumulate characters
        // The name will be extracted when we see a non-identifier character
        return;
    } else if (c == '(') {
        // Function name complete, extract it
        std::string funcName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->name = funcName;
        }
        ctx.state = STATE::FUNCTION_PARAMETERS_START;
        // Let the main loop handle the '(' character in the new state
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Function name complete, extract it
        std::string funcName = ctx.code.substr(ctx.stringStart, ctx.index - ctx.stringStart);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->name = funcName;
        }
        // Stay in this state waiting for '('
    } else {
        throw std::runtime_error("Expected '(' after function name: " + std::string(1, c));
    }
}

inline void handleStateFunctionParametersStart(ParserContext& ctx, char c) {
    if (c == 40) {  // ASCII for '('
        // Just consumed '(', now wait for parameters or ')'
        return;
    } else if (c == ')') {
        // Empty parameter list
        auto* paramList = new ParameterListNode(ctx.currentNode);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->parameters = paramList;
        } else if (auto* funcExpr = dynamic_cast<FunctionExpressionNode*>(ctx.currentNode)) {
            funcExpr->parameters = paramList;
        } else if (auto* arrowFunc = dynamic_cast<ArrowFunctionExpressionNode*>(ctx.currentNode)) {
            arrowFunc->parameters = paramList;
        }
        ctx.currentNode->children.push_back(paramList);
        ctx.state = STATE::FUNCTION_PARAMETERS_END;
    } else if (isalpha(c) || c == '_') {
        // Parameter name
        auto* paramList = new ParameterListNode(ctx.currentNode);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->parameters = paramList;
        } else if (auto* funcExpr = dynamic_cast<FunctionExpressionNode*>(ctx.currentNode)) {
            funcExpr->parameters = paramList;
        } else if (auto* arrowFunc = dynamic_cast<ArrowFunctionExpressionNode*>(ctx.currentNode)) {
            arrowFunc->parameters = paramList;
        }
        ctx.currentNode->children.push_back(paramList);
        ctx.currentNode = paramList;

        auto* param = new ParameterNode(ctx.currentNode);
        param->name = std::string(1, c);
        paramList->addParameter(param);
        ctx.currentNode = param;
        ctx.state = STATE::FUNCTION_PARAMETER_NAME;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected parameter name or ')': " + std::string(1, c));
    }
}

inline void handleStateFunctionParameterName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        if (auto* param = dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            param->name += c;
        }
    } else if (c == ':') {
        ctx.state = STATE::FUNCTION_PARAMETER_TYPE_ANNOTATION;
    } else if (c == ',' || c == ')') {
        // Parameter complete, move back to parameter list
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
    } else if (c == '=') {
        ctx.state = STATE::FUNCTION_PARAMETER_DEFAULT_VALUE;
    } else {
        throw std::runtime_error("Unexpected character in parameter name: " + std::string(1, c));
    }
}

inline void handleStateFunctionParameterTypeAnnotation(ParserContext& ctx, char c) {
    // Simplified type annotation handling - just expect basic types for now
    if (isalpha(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION;
        // This will need to be handled by the type annotation system
    } else {
        throw std::runtime_error("Expected type name: " + std::string(1, c));
    }
}

inline void handleStateFunctionParameterDefaultValue(ParserContext& ctx, char c) {
    // For now, skip default values - complex expression parsing needed
    if (c == ',' || c == ')') {
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
    } else {
        // Skip characters in default value for now
    }
}

inline void handleStateFunctionParameterSeparator(ParserContext& ctx, char c) {
    if (isalpha(c) || c == '_') {
        auto* paramList = dynamic_cast<ParameterListNode*>(ctx.currentNode);
        if (!paramList) {
            throw std::runtime_error("Expected parameter list context");
        }

        auto* param = new ParameterNode(ctx.currentNode);
        param->name = std::string(1, c);
        paramList->addParameter(param);
        ctx.currentNode = param;
        ctx.state = STATE::FUNCTION_PARAMETER_NAME;
    } else if (c == ')') {
        ctx.state = STATE::FUNCTION_PARAMETERS_END;
    } else {
        throw std::runtime_error("Expected parameter name or ')': " + std::string(1, c));
    }
}

inline void handleStateFunctionParametersEnd(ParserContext& ctx, char c) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        // Skip whitespace
        return;
    } else if (c == ':') {
        ctx.state = STATE::FUNCTION_RETURN_TYPE_ANNOTATION;
    } else if (c == '{') {
        // Function body start
        ctx.state = STATE::FUNCTION_BODY_START;
    } else if (c == '=' && ctx.currentNode->parent &&
               dynamic_cast<ArrowFunctionExpressionNode*>(ctx.currentNode->parent)) {
        // Arrow function
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::ARROW_FUNCTION_ARROW;
    } else {
        throw std::runtime_error("Expected ':' or '{' after parameters: " + std::string(1, c));
    }
}

inline void handleStateFunctionReturnTypeAnnotation(ParserContext& ctx, char c) {
    if (isalpha(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION;
        // This will set the return type on the function node
    } else {
        throw std::runtime_error("Expected return type: " + std::string(1, c));
    }
}

inline void handleStateFunctionBodyStart(ParserContext& ctx, char c) {
    // For now, just skip the body - complex statement parsing needed
    if (c == '}') {
        // End of function body
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else {
        // Skip body content for now
        ctx.state = STATE::FUNCTION_BODY;
    }
}

inline void handleStateFunctionBody(ParserContext& ctx, char c) {
    if (c == '}') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else {
        // Skip body content
    }
}

// Arrow function states
inline void handleStateArrowFunctionParameters(ParserContext& ctx, char c) {
    // Similar to function parameters but for arrow functions
    if (c == '(') {
        auto* arrowFunc = new ArrowFunctionExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(arrowFunc);
        ctx.currentNode = arrowFunc;
        ctx.state = STATE::FUNCTION_PARAMETERS_START;
    } else if (isalpha(c) || c == '_') {
        // Single parameter arrow function
        auto* arrowFunc = new ArrowFunctionExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(arrowFunc);
        ctx.currentNode = arrowFunc;

        auto* paramList = new ParameterListNode(ctx.currentNode);
        arrowFunc->parameters = paramList;
        ctx.currentNode->children.push_back(paramList);
        ctx.currentNode = paramList;

        auto* param = new ParameterNode(ctx.currentNode);
        param->name = std::string(1, c);
        paramList->addParameter(param);
        ctx.currentNode = param;
        ctx.state = STATE::FUNCTION_PARAMETER_NAME;
    } else {
        throw std::runtime_error("Expected '(' or parameter name for arrow function: " + std::string(1, c));
    }
}

inline void handleStateArrowFunctionArrow(ParserContext& ctx, char c) {
    if (c == '>') {
        ctx.state = STATE::ARROW_FUNCTION_BODY;
    } else {
        throw std::runtime_error("Expected '>' in arrow function: " + std::string(1, c));
    }
}

inline void handleStateArrowFunctionBody(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::FUNCTION_BODY_START;
    } else {
        // Expression body - for now just skip
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}
