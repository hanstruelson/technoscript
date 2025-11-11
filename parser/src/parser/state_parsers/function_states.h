#pragma once

#include <cctype>
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

// Function/for keyword detection
inline void handleStateBlockF(ParserContext& ctx, char c) {
    if (c == 'u') {
        // "function" - continue with function parsing
        ctx.state = STATE::BLOCK_FU;
    } else if (c == 'o') {
        // "for" - continue with for parsing
        ctx.state = STATE::BLOCK_FO;
    } else {
        throw std::runtime_error("Unexpected character after 'f': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFU(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::BLOCK_FUN;
    } else {
        throw std::runtime_error("Expected 'n' after 'fu': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFUN(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::BLOCK_FUNC;
    } else {
        throw std::runtime_error("Expected 'c' after 'fun': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFUNC(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::BLOCK_FUNCT;
    } else {
        throw std::runtime_error("Expected 't' after 'func': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFUNCT(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::BLOCK_FUNCTI;
    } else {
        throw std::runtime_error("Expected 'i' after 'funct': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFUNCTI(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::BLOCK_FUNCTIO;
    } else {
        throw std::runtime_error("Expected 'o' after 'functi': " + std::string(1, c));
    }
}

// Function keyword continuation
inline void handleStateBlockFUNCTIO(ParserContext& ctx, char c) {
    if (c == 'n') {
        ctx.state = STATE::BLOCK_FUNCTION;
    } else {
        throw std::runtime_error("Expected 'n' after 'functio': " + std::string(1, c));
    }
}

// Function keyword completion
inline void handleStateBlockFUNCTION(ParserContext& ctx, char c) {
    if (c == ' ') {
        auto* funcNode = new FunctionDeclarationNode(ctx.currentNode);
        ctx.currentNode->children.push_back(funcNode);
        ctx.currentNode = funcNode;

        // Set function and block scope to this function
        ctx.currentFunctionScope = funcNode;
        ctx.currentBlockScope = funcNode;

        ctx.state = STATE::FUNCTION_DECLARATION_NAME;
    } else {
        throw std::runtime_error("Expected ' ' after 'function': " + std::string(1, c));
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
    } else if (c == '<') {
        // Function name complete, extract it and check for generics
        std::string funcName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->name = funcName;
        }
        ctx.state = STATE::FUNCTION_GENERIC_PARAMETERS_START;
        // The '<' character has been consumed, so don't re-process it
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
        // Stay in this state waiting for '<' or '('
    } else {
        throw std::runtime_error("Expected '<', '(' or whitespace after function name: " + std::string(1, c));
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
    } else {
        // Start parsing parameter expression
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

        // Create first parameter and start parsing its pattern
        auto* param = new ParameterNode(ctx.currentNode);
        paramList->addParameter(param);
        ctx.currentNode = param;

        // Check for destructuring patterns first
        if (c == '[') {
            // Array destructuring parameter
            auto* arrayNode = new ArrayDestructuringNode(ctx.currentNode);
            param->pattern = arrayNode;
            ctx.currentNode = arrayNode;
            ctx.state = STATE::ARRAY_DESTRUCTURING_START;
        } else if (c == '{') {
            // Object destructuring parameter
            auto* objectNode = new ObjectDestructuringNode(ctx.currentNode);
            param->pattern = objectNode;
            ctx.currentNode = objectNode;
            ctx.state = STATE::OBJECT_DESTRUCTURING_START;
        } else {
            // Regular parameter - start expression parsing
            ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
            // Re-process this character as the start of the expression
            ctx.index--;
        }
    }
}

inline void handleStateFunctionParameterName(ParserContext& ctx, char c) {
    if (isalnum(c) || c == '_') {
        if (auto* param = dynamic_cast<ParameterNode*>(ctx.currentNode)) {
            if (!param->pattern) {
                param->pattern = new IdentifierExpressionNode(param, std::string(1, c));
                param->children.push_back(param->pattern);
            } else if (auto* ident = dynamic_cast<IdentifierExpressionNode*>(param->pattern)) {
                ident->name += c;
            }
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
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
    static bool startedParsing = false;

    // Handle whitespace before type name
    if (std::isspace(static_cast<unsigned char>(c))) {
        return;
    }

    // Start parsing type name
    if ((isalpha(c) || c == '_') && !startedParsing) {
        ctx.stringStart = ctx.index - 1; // Include current character
        startedParsing = true;
        // Continue in this state to accumulate type name
        return;
    }

    // Continue accumulating type name
    if (std::isalnum(c) || c == '_') {
        return;
    }

    // End of type name - process it
    if (c == ',' || c == ')') {
        std::string typeName = ctx.code.substr(ctx.stringStart, (ctx.index - 1) - ctx.stringStart);
        // Trim trailing whitespace
        typeName.erase(typeName.find_last_not_of(" \t\n\r\f\v") + 1);

        // Reset for next use
        startedParsing = false;

        // Set type annotation on parameter
        auto* param = dynamic_cast<ParameterNode*>(ctx.currentNode);
        if (param) {
            param->typeAnnotation = new TypeAnnotationNode(param);
            if (typeName == "string") {
                param->typeAnnotation->dataType = DataType::STRING;
            } else if (typeName == "int64") {
                param->typeAnnotation->dataType = DataType::INT64;
            } else if (typeName == "float64") {
                param->typeAnnotation->dataType = DataType::FLOAT64;
            } else {
                // Default to object for unknown types
                param->typeAnnotation->dataType = DataType::OBJECT;
            }
            param->children.push_back(param->typeAnnotation);
        }

        // Move back to parameter list
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
    } else {
        throw std::runtime_error("Unexpected character in parameter type annotation: " + std::string(1, c));
    }
}

inline void handleStateFunctionParameterDefaultValue(ParserContext& ctx, char c) {
    if (c == ',' || c == ')') {
        // End of default value
        ctx.currentNode = ctx.currentNode->parent;
        if (c == ',') {
            ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
        } else {
            ctx.state = STATE::FUNCTION_PARAMETERS_END;
        }
    } else {
        // Parse the default value expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character as the start of the expression
        ctx.index--;
    }
}

inline void handleStateFunctionParameterSeparator(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else if (c == ')') {
        ctx.state = STATE::FUNCTION_PARAMETERS_END;
    } else {
        auto* paramList = dynamic_cast<ParameterListNode*>(ctx.currentNode);
        if (!paramList) {
            throw std::runtime_error("Expected parameter list context");
        }

        // Create next parameter and start parsing its pattern
        auto* param = new ParameterNode(ctx.currentNode);
        paramList->addParameter(param);
        ctx.currentNode = param;

        // Check for destructuring patterns first
        if (c == '[') {
            // Array destructuring parameter
            auto* arrayNode = new ArrayDestructuringNode(ctx.currentNode);
            param->pattern = arrayNode;
            ctx.currentNode = arrayNode;
            ctx.state = STATE::ARRAY_DESTRUCTURING_START;
        } else if (c == '{') {
            // Object destructuring parameter
            auto* objectNode = new ObjectDestructuringNode(ctx.currentNode);
            param->pattern = objectNode;
            ctx.currentNode = objectNode;
            ctx.state = STATE::OBJECT_DESTRUCTURING_START;
        } else {
            // Regular parameter - start expression parsing
            ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
            // Re-process this character as the start of the expression
            ctx.index--;
        }
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
    if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace before return type
        return;
    } else if (isalpha(c)) {
        ctx.stringStart = ctx.index;
        ctx.state = STATE::TYPE_ANNOTATION;
        // This will set the return type on the function node
    } else {
        throw std::runtime_error("Expected return type: " + std::string(1, c));
    }
}

inline void handleStateFunctionBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Create function body block
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* funcNode = dynamic_cast<FunctionDeclarationNode*>(ctx.currentNode)) {
            funcNode->body = block;
        } else if (auto* funcExpr = dynamic_cast<FunctionExpressionNode*>(ctx.currentNode)) {
            funcExpr->body = block;
        } else if (auto* arrowFunc = dynamic_cast<ArrowFunctionExpressionNode*>(ctx.currentNode)) {
            arrowFunc->body = block;
        }
        ctx.currentNode->children.push_back(block);
        ctx.currentNode = block;

        // Set block scope to this block
        ctx.currentBlockScope = block;

        ctx.state = STATE::BLOCK;
    } else {
        throw std::runtime_error("Expected '{' for function body: " + std::string(1, c));
    }
}

inline void handleStateFunctionBody(ParserContext& ctx, char c) {
    if (c == '}') {
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::BLOCK;
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
        param->pattern = new IdentifierExpressionNode(param, std::string(1, c));
        param->children.push_back(param->pattern);
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
        // Block body
        ctx.state = STATE::FUNCTION_BODY_START;
    } else {
        // Expression body - parse the expression starting with this character
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character as the start of the expression
        ctx.index--;
    }
}

// Function expression handlers
inline void handleStateFunctionExpressionStart(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::BLOCK_F;
        // Re-process this character
        ctx.index--;
    } else {
        throw std::runtime_error("Expected 'f' for function expression: " + std::string(1, c));
    }
}

inline void handleStateFunctionExpressionParametersStart(ParserContext& ctx, char c) {
    if (c == '(') {
        auto* funcExpr = new FunctionExpressionNode(ctx.currentNode);
        ctx.currentNode->children.push_back(funcExpr);
        ctx.currentNode = funcExpr;
        ctx.state = STATE::FUNCTION_PARAMETERS_START;
        // Re-process this character
        ctx.index--;
    } else {
        throw std::runtime_error("Expected '(' for function expression parameters: " + std::string(1, c));
    }
}

inline void handleStateFunctionExpressionParameterName(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_PARAMETER_NAME;
    ctx.index--;
}

inline void handleStateFunctionExpressionParameterTypeAnnotation(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_PARAMETER_TYPE_ANNOTATION;
    ctx.index--;
}

inline void handleStateFunctionExpressionParameterDefaultValue(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_PARAMETER_DEFAULT_VALUE;
    ctx.index--;
}

inline void handleStateFunctionExpressionParameterSeparator(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_PARAMETER_SEPARATOR;
    ctx.index--;
}

inline void handleStateFunctionExpressionParametersEnd(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_PARAMETERS_END;
    ctx.index--;
}

inline void handleStateFunctionExpressionReturnTypeAnnotation(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_RETURN_TYPE_ANNOTATION;
    ctx.index--;
}

inline void handleStateFunctionExpressionBodyStart(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_BODY_START;
    ctx.index--;
}

inline void handleStateFunctionExpressionBody(ParserContext& ctx, char c) {
    ctx.state = STATE::FUNCTION_BODY;
    ctx.index--;
}
