#pragma once

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "expression_builder.h"

inline void closeExpression(ParserContext& ctx, char c) {
    while (ctx.currentNode->nodeType != ASTNodeType::EXPRESSION) {
        ctx.currentNode = ctx.currentNode->parent;
    }
    ctx.currentNode = ctx.currentNode->parent;
}