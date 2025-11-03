#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>

#include "state.h"
#include "state_parsers/state_handlers.h"

inline void printAst(const ASTNode* node, int indent = 0) {
    if (!node) {
        return;
    }
    node->print(std::cout, indent);
}

inline void reportParseError(const std::string& code, std::size_t index, const std::string& message, STATE state) {
    if (code.empty()) {
        std::cerr << "\nParse error: " << message << " while in state " << stateToString(state) << " at line 1, column 1\n^\n";
        return;
    }
    index = std::min(index, code.size() - 1);
    std::size_t lineStart = code.rfind('\n', index);
    lineStart = lineStart == std::string::npos ? 0 : lineStart + 1;
    std::size_t lineEnd = code.find('\n', index);
    lineEnd = lineEnd == std::string::npos ? code.size() : lineEnd;
    std::size_t lineNumber = static_cast<std::size_t>(std::count(code.begin(), code.begin() + index, '\n')) + 1;
    std::size_t column = index - lineStart;
    std::string line = code.substr(lineStart, lineEnd - lineStart);
    std::cerr << "\nParse error: " << message << " while in state " << stateToString(state) << " at line " << lineNumber << ", column " << column + 1 << '\n';
    std::cerr << line << '\n';
    std::cerr << std::string(column, ' ') << "^\n";
}

inline void parse(const std::string& code) {
    auto* root = new ASTNode(nullptr);
    ParserContext ctx(code, root);

    for (std::size_t i = 0; i < code.length(); ++i) {
        try {
            char c = code[i];
            std::cout << c;
            ctx.index = i;
            switch (ctx.state) {
                case STATE::NONE:
                    handleStateNone(ctx, c);
                    break;
                case STATE::NONE_V:
                    handleStateNoneV(ctx, c);
                    break;
                case STATE::NONE_VA:
                    handleStateNoneVA(ctx, c);
                    break;
                case STATE::NONE_VAR:
                    handleStateNoneVAR(ctx, c);
                    break;
                case STATE::NONE_C:
                    handleStateNoneC(ctx, c);
                    break;
                case STATE::NONE_CO:
                    handleStateNoneCO(ctx, c);
                    break;
                case STATE::NONE_CON:
                    handleStateNoneCON(ctx, c);
                    break;
                case STATE::NONE_CONS:
                    handleStateNoneCONS(ctx, c);
                    break;
                case STATE::NONE_CONST:
                    handleStateNoneCONST(ctx, c);
                    break;
                case STATE::NONE_L:
                    handleStateNoneL(ctx, c);
                    break;
                case STATE::NONE_LE:
                    handleStateNoneLE(ctx, c);
                    break;
                case STATE::NONE_LET:
                    handleStateNoneLET(ctx, c);
                    break;
                case STATE::NONE_F:
                    handleStateNoneF(ctx, c);
                    break;

                case STATE::EXPECT_IDENTIFIER:
                    handleStateExpectIdentifier(ctx, c);
                    break;
                case STATE::IDENTIFIER_NAME:
                    handleStateIdentifierName(ctx, c);
                    break;
                case STATE::VARIABLE_CREATE_IDENTIFIER_COMPLETE:
                    handleStateVariableCreateIdentifierComplete(ctx, c);
                    break;
                case STATE::IDENTIFIER_COMPLETE:
                    handleStateIdentifierComplete(ctx, c);
                    break;
                case STATE::EXPECT_TYPE_ANNOTATION:
                    handleStateExpectTypeAnnotation(ctx, c);
                    break;
                case STATE::TYPE_ANNOTATION:
                    handleStateTypeAnnotation(ctx, c);
                    break;
                case STATE::EXPECT_EQUALS:
                    handleStateExpectEquals(ctx, c);
                    break;
                case STATE::EXPRESSION_EXPECT_OPERAND:
                    handleStateExpressionExpectOperand(ctx, c);
                    break;
                case STATE::EXPRESSION_AFTER_OPERAND:
                    handleStateExpressionAfterOperand(ctx, c);
                    break;
                case STATE::EXPRESSION_AFTER_OPERAND_NEW_LINE:
                    handleStateExpressionAfterOperandNewLine(ctx, c);
                    break;
                case STATE::EXPRESSION_NUMBER:
                    handleStateExpressionNumber(ctx, c);
                    break;
                case STATE::EXPRESSION_IDENTIFIER:
                    handleStateExpressionIdentifier(ctx, c);
                    break;
                case STATE::EXPRESSION_SINGLE_QUOTE:
                    handleStateExpressionSingleQuote(ctx, c);
                    break;
                case STATE::EXPRESSION_SINGLE_QUOTE_ESCAPE:
                    handleStateExpressionSingleQuoteEscape(ctx, c);
                    break;
                case STATE::EXPRESSION_DOUBLE_QUOTE:
                    handleStateExpressionDoubleQuote(ctx, c);
                    break;
                case STATE::EXPRESSION_DOUBLE_QUOTE_ESCAPE:
                    handleStateExpressionDoubleQuoteEscape(ctx, c);
                    break;
                case STATE::EXPRESSION_PLUS:
                    handleStateExpressionPlus(ctx, c);
                    break;
                case STATE::EXPRESSION_MINUS:
                    handleStateExpressionMinus(ctx, c);
                    break;
                case STATE::EXPECT_IMMEDIATE_IDENTIFIER:
                    handleStateExpectImmediateIdentifier(ctx, c);
                    break;
                case STATE::FUNCTION_DECLARATION_NAME:
                    handleStateFunctionDeclarationName(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETERS_START:
                    handleStateFunctionParametersStart(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETER_NAME:
                    handleStateFunctionParameterName(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETER_TYPE_ANNOTATION:
                    handleStateFunctionParameterTypeAnnotation(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETER_DEFAULT_VALUE:
                    handleStateFunctionParameterDefaultValue(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETER_SEPARATOR:
                    handleStateFunctionParameterSeparator(ctx, c);
                    break;
                case STATE::FUNCTION_PARAMETERS_END:
                    handleStateFunctionParametersEnd(ctx, c);
                    break;
                case STATE::FUNCTION_RETURN_TYPE_ANNOTATION:
                    handleStateFunctionReturnTypeAnnotation(ctx, c);
                    break;
                case STATE::FUNCTION_BODY_START:
                    handleStateFunctionBodyStart(ctx, c);
                    break;
                case STATE::FUNCTION_BODY:
                    handleStateFunctionBody(ctx, c);
                    break;
                case STATE::ARROW_FUNCTION_PARAMETERS:
                    handleStateArrowFunctionParameters(ctx, c);
                    break;
                case STATE::ARROW_FUNCTION_ARROW:
                    handleStateArrowFunctionArrow(ctx, c);
                    break;
                case STATE::ARROW_FUNCTION_BODY:
                    handleStateArrowFunctionBody(ctx, c);
                    break;
                case STATE::ARRAY_LITERAL_START:
                    handleStateArrayLiteralStart(ctx, c);
                    break;
                case STATE::ARRAY_LITERAL_ELEMENT:
                    handleStateArrayLiteralElement(ctx, c);
                    break;
                case STATE::ARRAY_LITERAL_SEPARATOR:
                    handleStateArrayLiteralSeparator(ctx, c);
                    break;
                case STATE::OBJECT_LITERAL_START:
                    handleStateObjectLiteralStart(ctx, c);
                    break;
                case STATE::OBJECT_LITERAL_PROPERTY_KEY:
                    handleStateObjectLiteralPropertyKey(ctx, c);
                    break;
                case STATE::OBJECT_LITERAL_PROPERTY_COLON:
                    handleStateObjectLiteralPropertyColon(ctx, c);
                    break;
                case STATE::OBJECT_LITERAL_PROPERTY_VALUE:
                    handleStateObjectLiteralPropertyValue(ctx, c);
                    break;
                case STATE::OBJECT_LITERAL_SEPARATOR:
                    handleStateObjectLiteralSeparator(ctx, c);
                    break;
                case STATE::NONE_I:
                    handleStateNoneI(ctx, c);
                    break;
                case STATE::IF_CONDITION_START:
                    handleStateIfConditionStart(ctx, c);
                    break;
                case STATE::IF_CONSEQUENT:
                    handleStateIfConsequent(ctx, c);
                    break;
                case STATE::IF_ALTERNATE_START:
                    handleStateIfAlternateStart(ctx, c);
                    break;
                case STATE::IF_ALTERNATE:
                    handleStateIfAlternate(ctx, c);
                    break;
                case STATE::BLOCK_STATEMENT_START:
                    handleStateBlockStatementStart(ctx, c);
                    break;
                case STATE::BLOCK_STATEMENT_BODY:
                    handleStateBlockStatementBody(ctx, c);
                    break;
                case STATE::NONE_W:
                    handleStateNoneW(ctx, c);
                    break;
                case STATE::NONE_WH:
                    handleStateNoneWH(ctx, c);
                    break;
                case STATE::NONE_WHI:
                    handleStateNoneWHI(ctx, c);
                    break;
                case STATE::NONE_WHIL:
                    handleStateNoneWHIL(ctx, c);
                    break;
                case STATE::NONE_WHILE:
                    handleStateNoneWHILE(ctx, c);
                    break;
                case STATE::WHILE_CONDITION_START:
                    handleStateWhileConditionStart(ctx, c);
                    break;
                case STATE::WHILE_BODY:
                    handleStateWhileBody(ctx, c);
                    break;
            }
        } catch (const std::exception& e) {
            reportParseError(code, i, e.what(), ctx.state);
            delete root;
            return;
        } catch (...) {
            reportParseError(code, i, "Unknown parser error", ctx.state);
            delete root;
            return;
        }
    }
    printAst(root);
    delete root;
}
