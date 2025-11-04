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

    ctx.index = 0;
    while (ctx.index < code.length()) {
        try {
            char c = code[ctx.index++];
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
                case STATE::NONE_FU:
                    handleStateNoneFU(ctx, c);
                    break;
                case STATE::NONE_FUN:
                    handleStateNoneFUN(ctx, c);
                    break;
                case STATE::NONE_FUNC:
                    handleStateNoneFUNC(ctx, c);
                    break;
                case STATE::NONE_FUNCT:
                    handleStateNoneFUNCT(ctx, c);
                    break;
                case STATE::NONE_FUNCTI:
                    handleStateNoneFUNCTI(ctx, c);
                    break;
                case STATE::NONE_FUNCTIO:
                    handleStateNoneFUNCTIO(ctx, c);
                    break;
                case STATE::NONE_FUNCTION:
                    handleStateNoneFUNCTION(ctx, c);
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
                case STATE::FUNCTION_PARAMETER_COMPLETE:
                    handleStateFunctionParameterComplete(ctx, c);
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
                case STATE::TYPE_GENERIC_PARAMETERS_START:
                    handleStateTypeGenericParametersStart(ctx, c);
                    break;
                case STATE::TYPE_GENERIC_PARAMETER_NAME:
                    handleStateTypeGenericParameterName(ctx, c);
                    break;
                case STATE::TYPE_GENERIC_PARAMETER_SEPARATOR:
                    handleStateTypeGenericParameterSeparator(ctx, c);
                    break;
                case STATE::TYPE_GENERIC_PARAMETERS_END:
                    handleStateTypeGenericParametersEnd(ctx, c);
                    break;
                case STATE::TYPE_GENERIC_TYPE_START:
                    handleStateTypeGenericTypeStart(ctx, c);
                    break;
                case STATE::TYPE_GENERIC_TYPE_ARGUMENTS:
                    handleStateTypeGenericTypeArguments(ctx, c);
                    break;
                case STATE::FUNCTION_GENERIC_PARAMETERS_START:
                    handleStateFunctionGenericParametersStart(ctx, c);
                    break;
                case STATE::FUNCTION_GENERIC_PARAMETER_NAME:
                    handleStateFunctionGenericParameterName(ctx, c);
                    break;
                case STATE::FUNCTION_GENERIC_PARAMETER_SEPARATOR:
                    handleStateFunctionGenericParameterSeparator(ctx, c);
                    break;
                case STATE::FUNCTION_GENERIC_PARAMETERS_END:
                    handleStateFunctionGenericParametersEnd(ctx, c);
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
                case STATE::EXPRESSION_LESS:
                    handleStateExpressionLess(ctx, c);
                    break;
                case STATE::EXPRESSION_GREATER:
                    handleStateExpressionGreater(ctx, c);
                    break;
                case STATE::EXPRESSION_EQUALS:
                    handleStateExpressionEquals(ctx, c);
                    break;
                case STATE::EXPRESSION_EQUALS_DOUBLE:
                    handleStateExpressionEqualsDouble(ctx, c);
                    break;
                case STATE::EXPRESSION_NOT:
                    handleStateExpressionNot(ctx, c);
                    break;
                case STATE::EXPRESSION_NOT_EQUALS:
                    handleStateExpressionNotEquals(ctx, c);
                    break;
                case STATE::EXPRESSION_AND:
                    handleStateExpressionAnd(ctx, c);
                    break;
                case STATE::EXPRESSION_OR:
                    handleStateExpressionOr(ctx, c);
                    break;
                // New states for missing TypeScript features
                case STATE::EXPRESSION_PLUS_PLUS:
                    handleStateExpressionPlusPlus(ctx, c);
                    break;
                case STATE::EXPRESSION_MINUS_MINUS:
                    handleStateExpressionMinusMinus(ctx, c);
                    break;
                case STATE::EXPRESSION_LOGICAL_NOT:
                    handleStateExpressionLogicalNot(ctx, c);
                    break;
                case STATE::EXPRESSION_UNARY_PLUS:
                    handleStateExpressionUnaryPlus(ctx, c);
                    break;
                case STATE::EXPRESSION_UNARY_MINUS:
                    handleStateExpressionUnaryMinus(ctx, c);
                    break;
                case STATE::EXPRESSION_BITWISE_NOT:
                    handleStateExpressionBitwiseNot(ctx, c);
                    break;
                case STATE::EXPRESSION_EXPONENT:
                    handleStateExpressionExponent(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_AND:
                    handleStateExpressionBitAnd(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_OR:
                    handleStateExpressionBitOr(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_XOR:
                    handleStateExpressionBitXor(ctx, c);
                    break;
                case STATE::EXPRESSION_LEFT_SHIFT:
                    handleStateExpressionLeftShift(ctx, c);
                    break;
                case STATE::EXPRESSION_RIGHT_SHIFT:
                    handleStateExpressionRightShift(ctx, c);
                    break;
                case STATE::EXPRESSION_UNSIGNED_RIGHT_SHIFT:
                    handleStateExpressionUnsignedRightShift(ctx, c);
                    break;
                case STATE::EXPRESSION_ADD_ASSIGN:
                    handleStateExpressionAddAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_SUBTRACT_ASSIGN:
                    handleStateExpressionSubtractAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_MULTIPLY_ASSIGN:
                    handleStateExpressionMultiplyAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_DIVIDE_ASSIGN:
                    handleStateExpressionDivideAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_MODULO_ASSIGN:
                    handleStateExpressionModuloAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_EXPONENT_ASSIGN:
                    handleStateExpressionExponentAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_LEFT_SHIFT_ASSIGN:
                    handleStateExpressionLeftShiftAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_RIGHT_SHIFT_ASSIGN:
                    handleStateExpressionRightShiftAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_UNSIGNED_RIGHT_SHIFT_ASSIGN:
                    handleStateExpressionUnsignedRightShiftAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_AND_ASSIGN:
                    handleStateExpressionBitAndAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_OR_ASSIGN:
                    handleStateExpressionBitOrAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_BIT_XOR_ASSIGN:
                    handleStateExpressionBitXorAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_AND_ASSIGN:
                    handleStateExpressionAndAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_OR_ASSIGN:
                    handleStateExpressionOrAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_NULLISH_ASSIGN:
                    handleStateExpressionNullishAssign(ctx, c);
                    break;
                case STATE::EXPRESSION_TEMPLATE_LITERAL_START:
                    handleStateExpressionTemplateLiteralStart(ctx, c);
                    break;
                case STATE::EXPRESSION_TEMPLATE_LITERAL:
                    handleStateExpressionTemplateLiteral(ctx, c);
                    break;
                case STATE::EXPRESSION_TEMPLATE_LITERAL_ESCAPE:
                    handleStateExpressionTemplateLiteralEscape(ctx, c);
                    break;
                case STATE::EXPRESSION_TEMPLATE_LITERAL_INTERPOLATION:
                    handleStateExpressionTemplateLiteralInterpolation(ctx, c);
                    break;
                case STATE::EXPRESSION_REGEXP_START:
                    handleStateExpressionRegExpStart(ctx, c);
                    break;
                case STATE::EXPRESSION_REGEXP:
                    handleStateExpressionRegExp(ctx, c);
                    break;
                case STATE::EXPRESSION_REGEXP_ESCAPE:
                    handleStateExpressionRegExpEscape(ctx, c);
                    break;
                case STATE::EXPRESSION_REGEXP_FLAGS:
                    handleStateExpressionRegExpFlags(ctx, c);
                    break;
                case STATE::NONE_D:
                    handleStateNoneD(ctx, c);
                    break;
                case STATE::NONE_DO:
                    handleStateNoneDO(ctx, c);
                    break;
                case STATE::NONE_DOW:
                    handleStateNoneDOW(ctx, c);
                    break;
                case STATE::NONE_DOWH:
                    handleStateNoneDOWH(ctx, c);
                    break;
                case STATE::NONE_DOWHI:
                    handleStateNoneDOWHI(ctx, c);
                    break;
                case STATE::NONE_DOWHIL:
                    handleStateNoneDOWHIL(ctx, c);
                    break;
                case STATE::NONE_DOWHILE:
                    handleStateNoneDOWHILE(ctx, c);
                    break;
                case STATE::DO_BODY_START:
                    handleStateDoBodyStart(ctx, c);
                    break;
                case STATE::DO_BODY:
                    handleStateDoBody(ctx, c);
                    break;
                case STATE::DO_WHILE_CONDITION_START:
                    handleStateDoWhileConditionStart(ctx, c);
                    break;
                case STATE::NONE_FO:
                    handleStateNoneFO(ctx, c);
                    break;
                case STATE::NONE_FOR:
                    handleStateNoneFOR(ctx, c);
                    break;
                case STATE::FOR_INIT_START:
                    handleStateForInitStart(ctx, c);
                    break;
                case STATE::FOR_INIT:
                    handleStateForInit(ctx, c);
                    break;
                case STATE::FOR_TEST_START:
                    handleStateForTestStart(ctx, c);
                    break;
                case STATE::FOR_TEST:
                    handleStateForTest(ctx, c);
                    break;
                case STATE::FOR_UPDATE_START:
                    handleStateForUpdateStart(ctx, c);
                    break;
                case STATE::FOR_UPDATE:
                    handleStateForUpdate(ctx, c);
                    break;
                case STATE::FOR_BODY_START:
                    handleStateForBodyStart(ctx, c);
                    break;
                case STATE::FOR_BODY:
                    handleStateForBody(ctx, c);
                    break;
                case STATE::NONE_S:
                    handleStateNoneS(ctx, c);
                    break;
                case STATE::NONE_SW:
                    handleStateNoneSW(ctx, c);
                    break;
                case STATE::NONE_SWI:
                    handleStateNoneSWI(ctx, c);
                    break;
                case STATE::NONE_SWIT:
                    handleStateNoneSWIT(ctx, c);
                    break;
                case STATE::NONE_SWITC:
                    handleStateNoneSWITC(ctx, c);
                    break;
                case STATE::NONE_SWITCH:
                    handleStateNoneSWITCH(ctx, c);
                    break;
                case STATE::SWITCH_CONDITION_START:
                    handleStateSwitchConditionStart(ctx, c);
                    break;
                case STATE::SWITCH_BODY_START:
                    handleStateSwitchBodyStart(ctx, c);
                    break;
                case STATE::SWITCH_BODY:
                    handleStateSwitchBody(ctx, c);
                    break;
                case STATE::SWITCH_CASE_START:
                    handleStateSwitchCaseStart(ctx, c);
                    break;
                case STATE::SWITCH_CASE:
                    handleStateSwitchCase(ctx, c);
                    break;
                case STATE::SWITCH_DEFAULT_START:
                    handleStateSwitchDefaultStart(ctx, c);
                    break;
                case STATE::SWITCH_DEFAULT:
                    handleStateSwitchDefault(ctx, c);
                    break;
                case STATE::NONE_E:
                    handleStateNoneE(ctx, c);
                    break;
                case STATE::NONE_EL:
                    handleStateNoneEL(ctx, c);
                    break;
                case STATE::NONE_ELS:
                    handleStateNoneELS(ctx, c);
                    break;
                case STATE::NONE_ELSE:
                    handleStateNoneELSE(ctx, c);
                    break;
                case STATE::NONE_T:
                    handleStateNoneT(ctx, c);
                    break;
                case STATE::NONE_TR:
                    handleStateNoneTR(ctx, c);
                    break;
                case STATE::NONE_TRY:
                    handleStateNoneTRY(ctx, c);
                    break;
                case STATE::TRY_BODY_START:
                    handleStateTryBodyStart(ctx, c);
                    break;
                case STATE::TRY_BODY:
                    handleStateTryBody(ctx, c);
                    break;
                case STATE::TRY_CATCH_START:
                    handleStateTryCatchStart(ctx, c);
                    break;
                case STATE::TRY_CATCH:
                    handleStateTryCatch(ctx, c);
                    break;
                case STATE::TRY_CATCH_PARAM_START:
                    handleStateTryCatchParamStart(ctx, c);
                    break;
                case STATE::TRY_CATCH_PARAM:
                    handleStateTryCatchParam(ctx, c);
                    break;
                case STATE::TRY_CATCH_BODY_START:
                    handleStateTryCatchBodyStart(ctx, c);
                    break;
                case STATE::TRY_CATCH_BODY:
                    handleStateTryCatchBody(ctx, c);
                    break;
                case STATE::TRY_FINALLY_START:
                    handleStateTryFinallyStart(ctx, c);
                    break;
                case STATE::TRY_FINALLY:
                    handleStateTryFinally(ctx, c);
                    break;
                case STATE::TRY_FINALLY_BODY_START:
                    handleStateTryFinallyBodyStart(ctx, c);
                    break;
                case STATE::TRY_FINALLY_BODY:
                    handleStateTryFinallyBody(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_START:
                    handleStateFunctionExpressionStart(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETERS_START:
                    handleStateFunctionExpressionParametersStart(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETER_NAME:
                    handleStateFunctionExpressionParameterName(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETER_TYPE_ANNOTATION:
                    handleStateFunctionExpressionParameterTypeAnnotation(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETER_DEFAULT_VALUE:
                    handleStateFunctionExpressionParameterDefaultValue(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETER_SEPARATOR:
                    handleStateFunctionExpressionParameterSeparator(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_PARAMETERS_END:
                    handleStateFunctionExpressionParametersEnd(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_RETURN_TYPE_ANNOTATION:
                    handleStateFunctionExpressionReturnTypeAnnotation(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_BODY_START:
                    handleStateFunctionExpressionBodyStart(ctx, c);
                    break;
                case STATE::FUNCTION_EXPRESSION_BODY:
                    handleStateFunctionExpressionBody(ctx, c);
                    break;
                case STATE::NONE_CL:
                    handleStateNoneCL(ctx, c);
                    break;
                case STATE::NONE_CLA:
                    handleStateNoneCLA(ctx, c);
                    break;
                case STATE::NONE_CLAS:
                    handleStateNoneCLAS(ctx, c);
                    break;
                case STATE::NONE_CLASS:
                    handleStateNoneCLASS(ctx, c);
                    break;
                case STATE::CLASS_DECLARATION_NAME:
                    handleStateClassDeclarationName(ctx, c);
                    break;
                case STATE::CLASS_EXTENDS_START:
                    handleStateClassExtendsStart(ctx, c);
                    break;
                case STATE::CLASS_EXTENDS_NAME:
                    handleStateClassExtendsName(ctx, c);
                    break;
                case STATE::CLASS_IMPLEMENTS_START:
                    handleStateClassImplementsStart(ctx, c);
                    break;
                case STATE::CLASS_IMPLEMENTS_NAME:
                    handleStateClassImplementsName(ctx, c);
                    break;
                case STATE::CLASS_IMPLEMENTS_SEPARATOR:
                    handleStateClassImplementsSeparator(ctx, c);
                    break;
                case STATE::CLASS_BODY_START:
                    handleStateClassBodyStart(ctx, c);
                    break;
                case STATE::CLASS_BODY:
                    handleStateClassBody(ctx, c);
                    break;
                case STATE::CLASS_STATIC_START:
                    handleStateClassStaticStart(ctx, c);
                    break;
                case STATE::CLASS_PROPERTY_KEY:
                    handleStateClassPropertyKey(ctx, c);
                    break;
                case STATE::CLASS_PROPERTY_TYPE:
                    handleStateClassPropertyType(ctx, c);
                    break;
                case STATE::CLASS_PROPERTY_INITIALIZER:
                    handleStateClassPropertyInitializer(ctx, c);
                    break;
                case STATE::CLASS_METHOD_PARAMETERS_START:
                    handleStateClassMethodParametersStart(ctx, c);
                    break;
                case STATE::CLASS_METHOD_PARAMETERS_END:
                    handleStateClassMethodParametersEnd(ctx, c);
                    break;
                case STATE::CLASS_METHOD_RETURN_TYPE:
                    handleStateClassMethodReturnType(ctx, c);
                    break;
                case STATE::CLASS_METHOD_BODY_START:
                    handleStateClassMethodBodyStart(ctx, c);
                    break;
                case STATE::CLASS_METHOD_BODY:
                    handleStateClassMethodBody(ctx, c);
                    break;
                // New class feature states
                case STATE::CLASS_ACCESS_MODIFIER_PUBLIC:
                    handleStateClassAccessModifierPublic(ctx, c);
                    break;
                case STATE::CLASS_ACCESS_MODIFIER_PRIVATE:
                    handleStateClassAccessModifierPrivate(ctx, c);
                    break;
                case STATE::CLASS_ACCESS_MODIFIER_PROTECTED:
                    handleStateClassAccessModifierProtected(ctx, c);
                    break;
                case STATE::CLASS_READONLY_MODIFIER:
                    handleStateClassReadonlyModifier(ctx, c);
                    break;
                case STATE::CLASS_ABSTRACT_MODIFIER:
                    handleStateClassAbstractModifier(ctx, c);
                    break;
                case STATE::CLASS_GETTER_START:
                    handleStateClassGetterStart(ctx, c);
                    break;
                case STATE::CLASS_SETTER_START:
                    handleStateClassSetterStart(ctx, c);
                    break;
                case STATE::CLASS_GETTER_NAME:
                    handleStateClassGetterName(ctx, c);
                    break;
                case STATE::CLASS_SETTER_NAME:
                    handleStateClassSetterName(ctx, c);
                    break;
                case STATE::CLASS_GETTER_PARAMETERS_START:
                    handleStateClassGetterParametersStart(ctx, c);
                    break;
                case STATE::CLASS_SETTER_PARAMETERS_START:
                    handleStateClassSetterParametersStart(ctx, c);
                    break;
                case STATE::CLASS_GETTER_BODY_START:
                    handleStateClassGetterBodyStart(ctx, c);
                    break;
                case STATE::CLASS_SETTER_BODY_START:
                    handleStateClassSetterBodyStart(ctx, c);
                    break;
                case STATE::CLASS_GETTER_BODY:
                    handleStateClassGetterBody(ctx, c);
                    break;
                case STATE::CLASS_SETTER_BODY:
                    handleStateClassSetterBody(ctx, c);
                    break;
                case STATE::NONE_I:
                    handleStateNoneI(ctx, c);
                    break;
                case STATE::NONE_IN:
                    handleStateNoneIN(ctx, c);
                    break;
                case STATE::NONE_INT:
                    handleStateNoneINT(ctx, c);
                    break;
                case STATE::NONE_INTE:
                    handleStateNoneINTE(ctx, c);
                    break;
                case STATE::NONE_INTER:
                    handleStateNoneINTER(ctx, c);
                    break;
                case STATE::NONE_INTERF:
                    handleStateNoneINTERF(ctx, c);
                    break;
                case STATE::NONE_INTERFA:
                    handleStateNoneINTERFA(ctx, c);
                    break;
                case STATE::NONE_INTERFAC:
                    handleStateNoneINTERFAC(ctx, c);
                    break;
                case STATE::NONE_INTERFACE:
                    handleStateNoneINTERFACE(ctx, c);
                    break;
                case STATE::INTERFACE_DECLARATION_NAME:
                    handleStateInterfaceDeclarationName(ctx, c);
                    break;
                case STATE::INTERFACE_EXTENDS_START:
                    handleStateInterfaceExtendsStart(ctx, c);
                    break;
                case STATE::INTERFACE_EXTENDS_NAME:
                    handleStateInterfaceExtendsName(ctx, c);
                    break;
                case STATE::INTERFACE_EXTENDS_SEPARATOR:
                    handleStateInterfaceExtendsSeparator(ctx, c);
                    break;
                case STATE::INTERFACE_BODY_START:
                    handleStateInterfaceBodyStart(ctx, c);
                    break;
                case STATE::INTERFACE_BODY:
                    handleStateInterfaceBody(ctx, c);
                    break;
                case STATE::INTERFACE_PROPERTY_KEY:
                    handleStateInterfacePropertyKey(ctx, c);
                    break;
                case STATE::INTERFACE_PROPERTY_TYPE:
                    handleStateInterfacePropertyType(ctx, c);
                    break;
                case STATE::INTERFACE_METHOD_PARAMETERS_START:
                    handleStateInterfaceMethodParametersStart(ctx, c);
                    break;
                case STATE::INTERFACE_METHOD_PARAMETERS_END:
                    handleStateInterfaceMethodParametersEnd(ctx, c);
                    break;
                case STATE::INTERFACE_METHOD_RETURN_TYPE:
                    handleStateInterfaceMethodReturnType(ctx, c);
                    break;
                case STATE::INTERFACE_PROPERTY_OPTIONAL:
                    handleStateInterfacePropertyOptional(ctx, c);
                    break;
                case STATE::INTERFACE_PROPERTY_READONLY:
                    handleStateInterfacePropertyReadonly(ctx, c);
                    break;

                case STATE::INTERFACE_INDEX_SIGNATURE_START:
                    handleStateInterfaceIndexSignatureStart(ctx, c);
                    break;
                case STATE::INTERFACE_INDEX_SIGNATURE_KEY:
                    handleStateInterfaceIndexSignatureKey(ctx, c);
                    break;
                case STATE::INTERFACE_INDEX_SIGNATURE_KEY_TYPE:
                    handleStateInterfaceIndexSignatureKeyType(ctx, c);
                    break;
                case STATE::INTERFACE_INDEX_SIGNATURE_VALUE_TYPE:
                    handleStateInterfaceIndexSignatureValueType(ctx, c);
                    break;
                case STATE::INTERFACE_INDEX_SIGNATURE_READONLY:
                    handleStateInterfaceIndexSignatureReadonly(ctx, c);
                    break;
                case STATE::INTERFACE_CALL_SIGNATURE_START:
                    handleStateInterfaceCallSignatureStart(ctx, c);
                    break;
                case STATE::INTERFACE_CALL_SIGNATURE_PARAMETERS_START:
                    handleStateInterfaceCallSignatureParametersStart(ctx, c);
                    break;
                case STATE::INTERFACE_CALL_SIGNATURE_PARAMETERS_END:
                    handleStateInterfaceCallSignatureParametersEnd(ctx, c);
                    break;
                case STATE::INTERFACE_CALL_SIGNATURE_RETURN_TYPE:
                    handleStateInterfaceCallSignatureReturnType(ctx, c);
                    break;
                case STATE::INTERFACE_CONSTRUCT_SIGNATURE_START:
                    handleStateInterfaceConstructSignatureStart(ctx, c);
                    break;
                case STATE::INTERFACE_CONSTRUCT_SIGNATURE_PARAMETERS_START:
                    handleStateInterfaceConstructSignatureParametersStart(ctx, c);
                    break;
                case STATE::INTERFACE_CONSTRUCT_SIGNATURE_PARAMETERS_END:
                    handleStateInterfaceConstructSignatureParametersEnd(ctx, c);
                    break;
                case STATE::INTERFACE_CONSTRUCT_SIGNATURE_RETURN_TYPE:
                    handleStateInterfaceConstructSignatureReturnType(ctx, c);
                    break;
                // Module import/export states
                case STATE::NONE_IM:
                    handleStateNoneIM(ctx, c);
                    break;
                case STATE::NONE_IMP:
                    handleStateNoneIMP(ctx, c);
                    break;
                case STATE::NONE_IMPO:
                    handleStateNoneIMPO(ctx, c);
                    break;
                case STATE::NONE_IMPOR:
                    handleStateNoneIMPOR(ctx, c);
                    break;
                case STATE::NONE_IMPORT:
                    handleStateNoneIMPORT(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIERS_START:
                    handleStateImportSpecifiersStart(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIER_NAME:
                    handleStateImportSpecifierName(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIER_AS:
                    handleStateImportSpecifierAs(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIER_LOCAL_NAME:
                    handleStateImportSpecifierLocalName(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIER_SEPARATOR:
                    handleStateImportSpecifierSeparator(ctx, c);
                    break;
                case STATE::IMPORT_SPECIFIERS_END:
                    handleStateImportSpecifiersEnd(ctx, c);
                    break;
                case STATE::IMPORT_FROM:
                    handleStateImportFrom(ctx, c);
                    break;
                case STATE::IMPORT_SOURCE_START:
                    handleStateImportSourceStart(ctx, c);
                    break;
                case STATE::IMPORT_SOURCE:
                    handleStateImportSource(ctx, c);
                    break;
                case STATE::IMPORT_SOURCE_END:
                    handleStateImportSourceEnd(ctx, c);
                    break;
                case STATE::NONE_EX:
                    handleStateNoneEX(ctx, c);
                    break;
                case STATE::NONE_EXP:
                    handleStateNoneEXP(ctx, c);
                    break;
                case STATE::NONE_EXPO:
                    handleStateNoneEXPO(ctx, c);
                    break;
                case STATE::NONE_EXPOR:
                    handleStateNoneEXPOR(ctx, c);
                    break;
                case STATE::NONE_EXPORT:
                    handleStateNoneEXPORT(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIERS_START:
                    handleStateExportSpecifiersStart(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIER_NAME:
                    handleStateExportSpecifierName(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIER_AS:
                    handleStateExportSpecifierAs(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIER_EXPORTED_NAME:
                    handleStateExportSpecifierExportedName(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIER_SEPARATOR:
                    handleStateExportSpecifierSeparator(ctx, c);
                    break;
                case STATE::EXPORT_SPECIFIERS_END:
                    handleStateExportSpecifiersEnd(ctx, c);
                    break;
                case STATE::EXPORT_FROM:
                    handleStateExportFrom(ctx, c);
                    break;
                case STATE::EXPORT_SOURCE_START:
                    handleStateExportSourceStart(ctx, c);
                    break;
                case STATE::EXPORT_SOURCE:
                    handleStateExportSource(ctx, c);
                    break;
                case STATE::EXPORT_SOURCE_END:
                    handleStateExportSourceEnd(ctx, c);
                    break;
                case STATE::EXPORT_DEFAULT:
                    handleStateExportDefault(ctx, c);
                    break;
                case STATE::EXPORT_ALL:
                    handleStateExportAll(ctx, c);
                    break;
                case STATE::EXPORT_DECLARATION:
                    handleStateExportDeclaration(ctx, c);
                    break;
                // Destructuring states
                case STATE::ARRAY_DESTRUCTURING_START:
                    handleStateArrayDestructuringStart(ctx, c);
                    break;
                case STATE::ARRAY_DESTRUCTURING_ELEMENT:
                    handleStateArrayDestructuringElement(ctx, c);
                    break;
                case STATE::ARRAY_DESTRUCTURING_SEPARATOR:
                    handleStateArrayDestructuringSeparator(ctx, c);
                    break;
                case STATE::ARRAY_DESTRUCTURING_REST:
                    handleStateArrayDestructuringRest(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_START:
                    handleStateObjectDestructuringStart(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_PROPERTY_KEY:
                    handleStateObjectDestructuringPropertyKey(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_PROPERTY_COLON:
                    handleStateObjectDestructuringPropertyColon(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_PROPERTY_VALUE:
                    handleStateObjectDestructuringPropertyValue(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_SEPARATOR:
                    handleStateObjectDestructuringSeparator(ctx, c);
                    break;
                case STATE::OBJECT_DESTRUCTURING_REST:
                    handleStateObjectDestructuringRest(ctx, c);
                    break;
                // Async/await states
                case STATE::NONE_A:
                    handleStateNoneA(ctx, c);
                    break;
                case STATE::NONE_AS:
                    handleStateNoneAS(ctx, c);
                    break;
                case STATE::NONE_ASY:
                    handleStateNoneASY(ctx, c);
                    break;
                case STATE::NONE_ASYN:
                    handleStateNoneASYN(ctx, c);
                    break;
                case STATE::NONE_ASYNC:
                    handleStateNoneASYNC(ctx, c);
                    break;
                case STATE::NONE_AW:
                    handleStateNoneAW(ctx, c);
                    break;
                case STATE::NONE_AWA:
                    handleStateNoneAWA(ctx, c);
                    break;
                case STATE::NONE_AWAI:
                    handleStateNoneAWAI(ctx, c);
                    break;
                case STATE::NONE_AWAIT:
                    handleStateNoneAWAIT(ctx, c);
                    break;
                case STATE::EXPRESSION_AWAIT:
                    handleStateExpressionAwait(ctx, c);
                    break;
                // Enum states
                case STATE::NONE_ENUM_E:
                    handleStateNoneEnumE(ctx, c);
                    break;
                case STATE::NONE_ENUM_EN:
                    handleStateNoneEnumEN(ctx, c);
                    break;
                case STATE::NONE_ENUM_ENU:
                    handleStateNoneEnumENU(ctx, c);
                    break;
                case STATE::ENUM_DECLARATION_NAME:
                    handleStateEnumDeclarationName(ctx, c);
                    break;
                case STATE::ENUM_BODY_START:
                    handleStateEnumBodyStart(ctx, c);
                    break;
                case STATE::ENUM_BODY:
                    handleStateEnumBody(ctx, c);
                    break;
                case STATE::ENUM_MEMBER_NAME:
                    handleStateEnumMemberName(ctx, c);
                    break;
                case STATE::ENUM_MEMBER_INITIALIZER:
                    handleStateEnumMemberInitializer(ctx, c);
                    break;
                case STATE::ENUM_MEMBER_SEPARATOR:
                    handleStateEnumMemberSeparator(ctx, c);
                    break;
            }
        } catch (const std::exception& e) {
            reportParseError(code, ctx.index - 1, e.what(), ctx.state);
            delete root;
            return;
        } catch (...) {
            reportParseError(code, ctx.index - 1, "Unknown parser error", ctx.state);
            delete root;
            return;
        }
    }
    printAst(root);
    delete root;
}
