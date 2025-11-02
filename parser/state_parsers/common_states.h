#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"

// Root state handler - entry point for parsing
inline void handleStateNone(ParserContext& ctx, char c) {
    if (c == 'v') {
        ctx.state = STATE::NONE_V;
    } else if (c == 'c') {
        ctx.state = STATE::NONE_C;
    } else if (c == 'l') {
        ctx.state = STATE::NONE_L;
    } else {
        throw std::runtime_error("Unexpected character" + std::string(1, c));
    }
}
