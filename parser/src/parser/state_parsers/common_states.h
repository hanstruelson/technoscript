#pragma once

#include <cctype>
#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Root state handler - entry point for parsing
inline void handleStateBlock(ParserContext& ctx, char c) {
    // Check if we need to exit a completed single-statement block
    if (ctx.currentNode && ctx.currentNode->nodeType == ASTNodeType::BLOCK_STATEMENT) {
        auto* block = static_cast<BlockStatement*>(ctx.currentNode);
        if (block->noBraces && ctx.currentNode->parent) {
            // Check if this is a control statement with a single statement block
            if (dynamic_cast<ControlStatement*>(ctx.currentNode->parent)) {
                // Check if the block has only one child (single statement)
                if (ctx.currentNode->children.size() == 1) {
                    // Move up the AST and close both the block and control statement
                    ctx.currentNode = ctx.currentNode->parent;
                    // The control statement is now current, so we can handle the next character
                }
            }
        }
    }

    // Check if we're expecting a body for a control statement
    if (ctx.currentNode && (ctx.currentNode->nodeType == ASTNodeType::IF_STATEMENT ||
                            ctx.currentNode->nodeType == ASTNodeType::WHILE_STATEMENT ||
                            ctx.currentNode->nodeType == ASTNodeType::FOR_STATEMENT ||
                            ctx.currentNode->nodeType == ASTNodeType::DO_WHILE_STATEMENT)) {
        if (c == '{') {
            // Block statement for body
            auto* block = new BlockStatement(ctx.currentNode, false); // noBraces = false
            ctx.currentNode->addChild(block);
            ctx.currentNode = block;
            ctx.state = STATE::BLOCK;
        } else if (std::isspace(static_cast<unsigned char>(c))) {
            // Skip whitespace
            return;
        } else {
            // Single statement - create noBraces block
            auto* block = new BlockStatement(ctx.currentNode, true); // noBraces = true
            ctx.currentNode->addChild(block);
            ctx.currentNode = block;
            ctx.state = STATE::BLOCK;
            ctx.index--; // Re-process this character
        }
        return;
    }

    // Check for 'e' keywords (else or export)
    if (c == 'e') {
        ctx.state = STATE::BLOCK_E;
    } else if (c == '}') {
        // End of block - pop back to parent
        if (ctx.currentNode && ctx.currentNode->nodeType == ASTNodeType::BLOCK_STATEMENT) {
            ctx.currentNode = ctx.currentNode->parent;
            // Restore block scope to parent block
            if (ctx.currentBlockScope && ctx.currentBlockScope->parent) {
                ctx.currentBlockScope = dynamic_cast<LexicalScopeNode*>(ctx.currentBlockScope->parent);
            }
            // Call onBlockComplete if the parent has it
            if (ctx.currentNode) {
                ctx.currentNode->onBlockComplete(ctx);
            }
        }
        return;
    } else if (c == ';') {
        // Statement terminator - check if we need to exit a single-statement block
        if (ctx.currentNode && ctx.currentNode->nodeType == ASTNodeType::BLOCK_STATEMENT) {
            // Check if this block was created for a single statement (has a control statement parent)
            if (ctx.currentNode->parent &&
                (ctx.currentNode->parent->nodeType == ASTNodeType::IF_STATEMENT ||
                 ctx.currentNode->parent->nodeType == ASTNodeType::WHILE_STATEMENT ||
                 ctx.currentNode->parent->nodeType == ASTNodeType::FOR_STATEMENT)) {
                // Exit the block
                ctx.currentNode = ctx.currentNode->parent;
                // Call onBlockComplete
                ctx.currentNode->onBlockComplete(ctx);
                return;
            }
        }
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
    } else if (c == 'v') {
        ctx.state = STATE::BLOCK_V;
    } else if (c == 'c') {
        ctx.state = STATE::BLOCK_C;
    } else if (c == 'l') {
        ctx.state = STATE::BLOCK_L;
    } else if (c == 'f') {
        ctx.state = STATE::BLOCK_F;
    } else if (c == 'i') {
        ctx.state = STATE::BLOCK_I;
    } else if (c == 'w') {
        ctx.state = STATE::BLOCK_W;
    } else if (c == 'd') {
        ctx.state = STATE::BLOCK_D;
    } else if (c == 's') {
        ctx.state = STATE::BLOCK_S;
    } else if (c == 't') {
        ctx.state = STATE::BLOCK_T;
    } else if (c == 'p') {
        ctx.state = STATE::BLOCK_P;
    } else if (c == 'g') {
        ctx.state = STATE::BLOCK_G;
    } else if (c == 'r') {
        ctx.state = STATE::BLOCK_R;
    } else if (c == 'n') {
        ctx.state = STATE::BLOCK_N;
    } else if (c == '/' && ctx.index < ctx.code.length() && ctx.code[ctx.index] == '/') {
        // Single-line comment: skip to end of line
        while (ctx.index < ctx.code.length() && ctx.code[ctx.index] != '\n') {
            ctx.index++;
        }
        // Stay in BLOCK state
    } else {
        // Handle expression starts at top level
        auto* expr = new ExpressionNode(ctx.currentNode);
        ctx.currentNode->addChild(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateBlockE(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::BLOCK_EL;
    } else if (c == 'x') {
        ctx.state = STATE::BLOCK_EX;
    } else if (c == 'n') {
        ctx.state = STATE::BLOCK_ENUM_E;
    } else {
        // Not 'else', 'export', or 'enum', treat as identifier starting with 'e'
        ctx.stringStart = ctx.index - 1; // 'e' is at index-1
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateBlockEL(ParserContext& ctx, char c) {
    if (c == 's') {
        ctx.state = STATE::BLOCK_ELS;
    } else {
        // Not 'else', treat as identifier starting with 'el'
        ctx.stringStart = ctx.index - 2; // 'e' is at index-2, 'l' is at index-1
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateBlockELS(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::BLOCK_ELSE;
    } else {
        // Not 'else', treat as identifier starting with 'els'
        ctx.stringStart = ctx.index - 3; // 'e' is at index-3, 'l' at index-2, 's' at index-1
        ctx.state = STATE::IDENTIFIER_NAME;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateBlockELSE(ParserContext& ctx, char c) {
    if (std::isspace(static_cast<unsigned char>(c))) {
        
    } else if (c == '{') {
        // Else block - create else clause with block
        auto* elseClause = new ElseClause(ctx.currentNode);
        auto* block = new BlockStatement(elseClause);
        elseClause->children.push_back(block);
        ctx.currentNode->children.push_back(elseClause);
        ctx.currentNode = block;
        ctx.state = STATE::BLOCK;
    } else if (c == 'i') {
        // Else if - create ElseIfClause
        auto* elseIfClause = new ElseIfClause(ctx.currentNode);
        ctx.currentNode->children.push_back(elseIfClause);
        ctx.currentNode = elseIfClause;
        ctx.state = STATE::IF_CONDITION_START;
        // Re-process 'i' for 'if' parsing
        ctx.index--;
    } else {
        // Single statement else - create else clause
        auto* elseClause = new ElseClause(ctx.currentNode);
        ctx.currentNode->children.push_back(elseClause);
        ctx.currentNode = elseClause;
        ctx.state = STATE::BLOCK;
        ctx.index--; // Re-process this character
    }
}
