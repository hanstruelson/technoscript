#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"
#include "../lib/expression_builder.h"

// Compact operator state handlers - consolidate similar operator handling

inline void handleStateExpressionLess(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_LESS_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_LESS);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionGreater(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_GREATER_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_GREATER);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionEquals(ParserContext& ctx, char c) {
    if (c == '=') {
        ctx.state = STATE::EXPRESSION_EQUALS_DOUBLE;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionEqualsDouble(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_STRICT_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionNot(ParserContext& ctx, char c) {
    if (c == '=') {
        ctx.state = STATE::EXPRESSION_NOT_EQUALS;
    } else {
        // For now, treat ! as invalid in expressions (logical NOT not implemented yet)
        throw std::runtime_error("Unexpected '!' in expression");
    }
}

inline void handleStateExpressionNotEquals(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_STRICT_NOT_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_NOT_EQUAL);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionAnd(ParserContext& ctx, char c) {
    if (c == '&') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_AND);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '&' for '&&' operator");
    }
}

inline void handleStateExpressionOr(ParserContext& ctx, char c) {
    if (c == '|') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_OR);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '|' for '||' operator");
    }
}

// New operator handlers for missing TypeScript features

inline void handleStateExpressionPlusPlus(ParserContext& ctx, char c) {
    if (c == '+') {
        // Second '+' - this is prefix increment
        auto* prefixNode = new PlusPlusPrefixExpressionNode(ctx.currentNode);
        addExpressionOperand(ctx, prefixNode);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        // Not '+', so this was just a single '+' followed by something else
        // Re-process this character as a unary plus
        ctx.state = STATE::EXPRESSION_UNARY_PLUS;
        ctx.index--;
    }
}

inline void handleStateExpressionMinusMinus(ParserContext& ctx, char c) {
    if (c == '-') {
        // Second '-' - this is prefix decrement
        auto* prefixNode = new MinusMinusPrefixExpressionNode(ctx.currentNode);
        addExpressionOperand(ctx, prefixNode);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        // Not '-', so this was just a single '-' followed by something else
        // Re-process this character as a unary minus
        ctx.state = STATE::EXPRESSION_UNARY_MINUS;
        ctx.index--;
    }
}

inline void handleStateExpressionLogicalNot(ParserContext& ctx, char c) {
    auto* notNode = new LogicalNotExpressionNode(ctx.currentNode);
    addExpressionOperand(ctx, notNode);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
}

inline void handleStateExpressionUnaryPlus(ParserContext& ctx, char c) {
    auto* plusNode = new UnaryPlusExpressionNode(ctx.currentNode);
    addExpressionOperand(ctx, plusNode);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
}

inline void handleStateExpressionUnaryMinus(ParserContext& ctx, char c) {
    auto* minusNode = new UnaryMinusExpressionNode(ctx.currentNode);
    addExpressionOperand(ctx, minusNode);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
}

inline void handleStateExpressionBitwiseNot(ParserContext& ctx, char c) {
    auto* notNode = new BitwiseNotExpressionNode(ctx.currentNode);
    addExpressionOperand(ctx, notNode);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
}

inline void handleStateExpressionExponent(ParserContext& ctx, char c) {
    if (c == '*') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_EXPONENT);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '*' for '**' operator");
    }
}

inline void handleStateExpressionBitAnd(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_AND_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_AND);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionBitOr(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_OR_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_OR);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionBitXor(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_XOR_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_XOR);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionLeftShift(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_LEFT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '<') {
        ctx.state = STATE::EXPRESSION_UNSIGNED_RIGHT_SHIFT;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_LEFT_SHIFT);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionRightShift(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_RIGHT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (c == '>') {
        ctx.state = STATE::EXPRESSION_UNSIGNED_RIGHT_SHIFT;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_RIGHT_SHIFT);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionUnsignedRightShift(ParserContext& ctx, char c) {
    if (c == '=') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_UNSIGNED_RIGHT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_UNSIGNED_RIGHT_SHIFT);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateExpressionAddAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_ADD_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionSubtractAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_SUBTRACT_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionMultiplyAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_MULTIPLY_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionDivideAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_DIVIDE_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionModuloAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_MODULO_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionExponentAssign(ParserContext& ctx, char c) {
    if (c == '*') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_EXPONENT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '*' for '**=' operator");
    }
}

inline void handleStateExpressionLeftShiftAssign(ParserContext& ctx, char c) {
    if (c == '<') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_LEFT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '<' for '<<=' operator");
    }
}

inline void handleStateExpressionRightShiftAssign(ParserContext& ctx, char c) {
    if (c == '>') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_RIGHT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '>' for '>>=' operator");
    }
}

inline void handleStateExpressionUnsignedRightShiftAssign(ParserContext& ctx, char c) {
    if (c == '>') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_UNSIGNED_RIGHT_SHIFT_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '>' for '>>>=' operator");
    }
}

inline void handleStateExpressionBitAndAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_AND_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionBitOrAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_OR_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionBitXorAssign(ParserContext& ctx, char c) {
    applyExpressionOperator(ctx, BinaryExpressionOperator::OP_BIT_XOR_ASSIGN);
    ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    // Re-process this character
    ctx.index--;
}

inline void handleStateExpressionAndAssign(ParserContext& ctx, char c) {
    if (c == '&') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_AND_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '&' for '&&=' operator");
    }
}

inline void handleStateExpressionOrAssign(ParserContext& ctx, char c) {
    if (c == '|') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_OR_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '|' for '||=' operator");
    }
}

inline void handleStateExpressionNullishAssign(ParserContext& ctx, char c) {
    if (c == '?') {
        applyExpressionOperator(ctx, BinaryExpressionOperator::OP_NULLISH_ASSIGN);
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else {
        throw std::runtime_error("Expected '?' for '??=' operator");
    }
}
