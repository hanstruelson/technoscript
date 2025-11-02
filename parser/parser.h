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
