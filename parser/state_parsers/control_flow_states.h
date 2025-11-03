#pragma once

#include <stdexcept>
#include <string>

#include "../lib/parser_context.h"
#include "../lib/ast.h"

// Control flow parsing states
inline void handleStateNoneI(ParserContext& ctx, char c) {
    if (c == 'f') {
        ctx.state = STATE::IF_CONDITION_START;
    } else if (c == 'n') {
        ctx.state = STATE::NONE_IN;
    } else {
        throw std::runtime_error("Expected 'f' or 'n' after 'i': " + std::string(1, c));
    }
}

inline void handleStateIfConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Check if we're parsing else if
        if (dynamic_cast<ElseIfClause*>(ctx.currentNode)) {
            // Else if - create condition expression as child
            auto* expr = new ExpressionNode(ctx.currentNode);
            ctx.currentNode->addChild(expr);
            ctx.currentNode = expr;
        } else {
            // Regular if - create if statement node
            auto* ifNode = new IfStatement(ctx.currentNode);
            ctx.currentNode->addChild(ifNode);
            ctx.currentNode = ifNode;

            // Start parsing condition expression
            auto* expr = new ExpressionNode(ctx.currentNode);
            ifNode->condition = expr;
            ifNode->addChild(expr);
            ctx.currentNode = expr;
        }
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'if': " + std::string(1, c));
    }
}

inline void handleStateIfConsequent(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for consequent
        auto* block = new BlockStatement(ctx.currentNode, false); // noBraces = false
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - create noBraces block
        auto* block = new BlockStatement(ctx.currentNode, true); // noBraces = true
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::NONE;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateIfAlternateStart(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Start of "else" - let main loop handle it
        ctx.state = STATE::NONE;
        ctx.index--; // Re-process this character
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // No else clause, end if statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
        ctx.index--; // Re-process this character
    }
}

inline void handleStateIfAlternate(ParserContext& ctx, char c) {
    if (c == 'i') {
        // Else if - create ElseIfClause
        auto* elseIfClause = new ElseIfClause(ctx.currentNode);
        ctx.currentNode->addChild(elseIfClause);
        ctx.currentNode = elseIfClause;
        ctx.state = STATE::IF_CONDITION_START;
        // Re-process 'i' for 'if' parsing
        ctx.index--;
    } else if (c == '{') {
        // Block statement for else
        auto* elseClause = new ElseClause(ctx.currentNode);
        auto* block = new BlockStatement(elseClause);
        elseClause->addChild(block);
        ctx.currentNode->addChild(elseClause);
        ctx.currentNode = block;
        // Stay in NONE state to parse block contents
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - create else clause with single statement
        ctx.state = STATE::NONE;
        ctx.index--; // Re-process this character
    }
}



// While loop parsing states
inline void handleStateNoneW(ParserContext& ctx, char c) {
    if (c == 'h') {
        ctx.state = STATE::NONE_WH;
    } else {
        throw std::runtime_error("Expected 'h' after 'w': " + std::string(1, c));
    }
}

inline void handleStateNoneWH(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_WHI;
    } else {
        throw std::runtime_error("Expected 'i' after 'wh': " + std::string(1, c));
    }
}

inline void handleStateNoneWHI(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::NONE_WHIL;
    } else {
        throw std::runtime_error("Expected 'l' after 'whi': " + std::string(1, c));
    }
}

inline void handleStateNoneWHIL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_WHILE;
    } else {
        throw std::runtime_error("Expected 'e' after 'whil': " + std::string(1, c));
    }
}

inline void handleStateNoneWHILE(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create while statement node and start parsing condition
        auto* whileNode = new WhileStatement(ctx.currentNode);
        ctx.currentNode->addChild(whileNode);
        ctx.currentNode = whileNode;

        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        whileNode->condition = expr;
        whileNode->addChild(expr);
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'while': " + std::string(1, c));
    }
}

inline void handleStateWhileConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* whileNode = dynamic_cast<WhileStatement*>(ctx.currentNode)) {
            whileNode->condition = expr;
            whileNode->addChild(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' for while condition: " + std::string(1, c));
    }
}

inline void handleStateWhileBody(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement
        auto* block = new BlockStatement(ctx.currentNode, false); // noBraces = false
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - create noBraces block
        auto* block = new BlockStatement(ctx.currentNode, true); // noBraces = true
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::NONE;
        // Re-process this character to parse the statement
        ctx.index--;
    }
}

// Do-while loop handlers
inline void handleStateNoneD(ParserContext& ctx, char c) {
    if (c == 'o') {
        ctx.state = STATE::NONE_DO;
    } else {
        throw std::runtime_error("Expected 'o' after 'd': " + std::string(1, c));
    }
}

inline void handleStateNoneDO(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create do-while statement node
        auto* doWhileNode = new DoWhileStatement(ctx.currentNode);
        ctx.currentNode->addChild(doWhileNode);
        ctx.currentNode = doWhileNode;

        // Start parsing body first (do-while executes body before condition)
        ctx.state = STATE::DO_BODY_START;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'do': " + std::string(1, c));
    }
}

inline void handleStateDoBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for body
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* doWhileNode = dynamic_cast<DoWhileStatement*>(ctx.currentNode)) {
            doWhileNode->body = block;
            doWhileNode->addChild(block);
        }
        ctx.currentNode = block;
        ctx.state = STATE::DO_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for do-while body: " + std::string(1, c));
    }
}

inline void handleStateDoBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of body, expect 'while'
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE_DOWHILE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip body content
        return;
    }
}

inline void handleStateDoWhileConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* doWhileNode = dynamic_cast<DoWhileStatement*>(ctx.currentNode)) {
            doWhileNode->condition = expr;
            doWhileNode->addChild(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' for do-while condition: " + std::string(1, c));
    }
}

// Missing do-while keyword handlers
inline void handleStateNoneDOW(ParserContext& ctx, char c) {
    if (c == 'h') {
        ctx.state = STATE::NONE_DOWH;
    } else {
        throw std::runtime_error("Expected 'h' after 'dow': " + std::string(1, c));
    }
}

inline void handleStateNoneDOWH(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_DOWHI;
    } else {
        throw std::runtime_error("Expected 'i' after 'dowh': " + std::string(1, c));
    }
}

inline void handleStateNoneDOWHI(ParserContext& ctx, char c) {
    if (c == 'l') {
        ctx.state = STATE::NONE_DOWHIL;
    } else {
        throw std::runtime_error("Expected 'l' after 'dowhi': " + std::string(1, c));
    }
}

inline void handleStateNoneDOWHIL(ParserContext& ctx, char c) {
    if (c == 'e') {
        ctx.state = STATE::NONE_DOWHILE;
    } else {
        throw std::runtime_error("Expected 'e' after 'dowhil': " + std::string(1, c));
    }
}

inline void handleStateNoneDOWHILE(ParserContext& ctx, char c) {
    if (c == '(') {
        // Start parsing condition expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* doWhileNode = dynamic_cast<DoWhileStatement*>(ctx.currentNode)) {
            doWhileNode->condition = expr;
            doWhileNode->addChild(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'while' in do-while: " + std::string(1, c));
    }
}

// For loop handlers
inline void handleStateNoneFO(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_FOR;
    } else {
        throw std::runtime_error("Expected 'r' after 'fo': " + std::string(1, c));
    }
}

inline void handleStateNoneFOR(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create for statement node
        auto* forNode = new ForStatement(ctx.currentNode);
        ctx.currentNode->addChild(forNode);
        ctx.currentNode = forNode;
        ctx.state = STATE::FOR_INIT_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'for': " + std::string(1, c));
    }
}

inline void handleStateForInitStart(ParserContext& ctx, char c) {
    if (c == ';') {
        // No initialization
        ctx.state = STATE::FOR_TEST_START;
    } else if (c == 'v') {
        // Variable declaration
        ctx.state = STATE::NONE_V;
        // Re-process this character
        ctx.index--;
    } else if (c == 'l') {
        // Let declaration
        ctx.state = STATE::NONE_L;
        // Re-process this character
        ctx.index--;
    } else if (c == 'c') {
        // Const declaration
        ctx.state = STATE::NONE_C;
        // Re-process this character
        ctx.index--;
    } else {
        // Expression initialization
        ctx.state = STATE::FOR_INIT;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForInit(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of initialization
        ctx.state = STATE::FOR_TEST_START;
    } else {
        // Parse initialization expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForTestStart(ParserContext& ctx, char c) {
    if (c == ';') {
        // No test condition
        ctx.state = STATE::FOR_UPDATE_START;
    } else {
        // Parse test expression
        ctx.state = STATE::FOR_TEST;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForTest(ParserContext& ctx, char c) {
    if (c == ';') {
        // End of test
        ctx.state = STATE::FOR_UPDATE_START;
    } else {
        // Parse test expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForUpdateStart(ParserContext& ctx, char c) {
    if (c == ')') {
        // No update
        ctx.state = STATE::FOR_BODY_START;
    } else {
        // Parse update expression
        ctx.state = STATE::FOR_UPDATE;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForUpdate(ParserContext& ctx, char c) {
    if (c == ')') {
        // End of update
        ctx.state = STATE::FOR_BODY_START;
    } else {
        // Parse update expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateForBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement
        auto* block = new BlockStatement(ctx.currentNode, false); // noBraces = false
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::FOR_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // Single statement - create noBraces block
        auto* block = new BlockStatement(ctx.currentNode, true); // noBraces = true
        ctx.currentNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::FOR_BODY;
        // Re-process this character to parse the statement
        ctx.index--;
    }
}

inline void handleStateForBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of for loop
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip body content
        return;
    }
}

// Switch statement handlers
inline void handleStateNoneS(ParserContext& ctx, char c) {
    if (c == 'w') {
        ctx.state = STATE::NONE_SW;
    } else {
        throw std::runtime_error("Expected 'w' after 's': " + std::string(1, c));
    }
}

inline void handleStateNoneSW(ParserContext& ctx, char c) {
    if (c == 'i') {
        ctx.state = STATE::NONE_SWI;
    } else {
        throw std::runtime_error("Expected 'i' after 'sw': " + std::string(1, c));
    }
}

inline void handleStateNoneSWI(ParserContext& ctx, char c) {
    if (c == 't') {
        ctx.state = STATE::NONE_SWIT;
    } else {
        throw std::runtime_error("Expected 't' after 'swi': " + std::string(1, c));
    }
}

inline void handleStateNoneSWIT(ParserContext& ctx, char c) {
    if (c == 'c') {
        ctx.state = STATE::NONE_SWITC;
    } else {
        throw std::runtime_error("Expected 'c' after 'swit': " + std::string(1, c));
    }
}

inline void handleStateNoneSWITC(ParserContext& ctx, char c) {
    if (c == 'h') {
        ctx.state = STATE::NONE_SWITCH;
    } else {
        throw std::runtime_error("Expected 'h' after 'switc': " + std::string(1, c));
    }
}

inline void handleStateNoneSWITCH(ParserContext& ctx, char c) {
    if (c == '(') {
        // Create switch statement node
        auto* switchNode = new SwitchStatement(ctx.currentNode);
        ctx.currentNode->addChild(switchNode);
        ctx.currentNode = switchNode;
        ctx.state = STATE::SWITCH_CONDITION_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' after 'switch': " + std::string(1, c));
    }
}

inline void handleStateSwitchConditionStart(ParserContext& ctx, char c) {
    if (c == '(') {
        // Start parsing discriminant expression
        auto* expr = new ExpressionNode(ctx.currentNode);
        if (auto* switchNode = dynamic_cast<SwitchStatement*>(ctx.currentNode)) {
            switchNode->discriminant = expr;
            switchNode->addChild(expr);
        }
        ctx.currentNode = expr;
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' for switch condition: " + std::string(1, c));
    }
}

inline void handleStateSwitchBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        ctx.state = STATE::SWITCH_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for switch body: " + std::string(1, c));
    }
}

inline void handleStateSwitchBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of switch
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    } else if (c == 'c') {
        // Case statement
        ctx.state = STATE::SWITCH_CASE_START;
        // Re-process this character
        ctx.index--;
    } else if (c == 'd') {
        // Default statement
        ctx.state = STATE::SWITCH_DEFAULT_START;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip other content
        return;
    }
}

inline void handleStateSwitchCaseStart(ParserContext& ctx, char c) {
    if (c == 'a') {
        // Continue with "case"
        return;
    } else if (c == 's') {
        // Continue with "case"
        return;
    } else if (c == 'e') {
        // Continue with "case"
        return;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'case' keyword: " + std::string(1, c));
    }
}

inline void handleStateSwitchCase(ParserContext& ctx, char c) {
    if (c == ':') {
        // End of case label
        ctx.state = STATE::SWITCH_BODY;
    } else {
        // Parse case expression
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateSwitchDefaultStart(ParserContext& ctx, char c) {
    if (c == 'e') {
        // Continue with "default"
        return;
    } else if (c == 'f') {
        // Continue with "default"
        return;
    } else if (c == 'a') {
        // Continue with "default"
        return;
    } else if (c == 'u') {
        // Continue with "default"
        return;
    } else if (c == 'l') {
        // Continue with "default"
        return;
    } else if (c == 't') {
        // Continue with "default"
        return;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'default' keyword: " + std::string(1, c));
    }
}

inline void handleStateSwitchDefault(ParserContext& ctx, char c) {
    if (c == ':') {
        // End of default label
        ctx.state = STATE::SWITCH_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected ':' after 'default': " + std::string(1, c));
    }
}

// Try-catch-finally handlers
inline void handleStateNoneT(ParserContext& ctx, char c) {
    if (c == 'r') {
        ctx.state = STATE::NONE_TR;
    } else {
        throw std::runtime_error("Expected 'r' after 't': " + std::string(1, c));
    }
}

inline void handleStateNoneTR(ParserContext& ctx, char c) {
    if (c == 'y') {
        ctx.state = STATE::NONE_TRY;
    } else {
        throw std::runtime_error("Expected 'y' after 'tr': " + std::string(1, c));
    }
}

inline void handleStateNoneTRY(ParserContext& ctx, char c) {
    if (c == '{') {
        // Create try statement node
        auto* tryNode = new TryStatement(ctx.currentNode);
        ctx.currentNode->addChild(tryNode);
        ctx.currentNode = tryNode;

        // Start parsing try block
        auto* block = new BlockStatement(ctx.currentNode);
        tryNode->block = block;
        tryNode->addChild(block);
        ctx.currentNode = block;
        ctx.state = STATE::TRY_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' after 'try': " + std::string(1, c));
    }
}

inline void handleStateTryBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for try body
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* tryNode = dynamic_cast<TryStatement*>(ctx.currentNode)) {
            tryNode->block = block;
            tryNode->addChild(block);
        }
        ctx.currentNode = block;
        ctx.state = STATE::TRY_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for try body: " + std::string(1, c));
    }
}

inline void handleStateTryBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of try block
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::TRY_CATCH_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip body content
        return;
    }
}

inline void handleStateTryCatchStart(ParserContext& ctx, char c) {
    if (c == 'c') {
        // Catch clause
        ctx.state = STATE::TRY_CATCH;
        // Re-process this character
        ctx.index--;
    } else if (c == 'f') {
        // Finally clause
        ctx.state = STATE::TRY_FINALLY;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // No catch or finally, end try statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}

inline void handleStateTryCatch(ParserContext& ctx, char c) {
    if (c == 'h') {
        // Continue with "catch"
        return;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'catch' keyword: " + std::string(1, c));
    }
}

inline void handleStateTryCatchParamStart(ParserContext& ctx, char c) {
    if (c == '(') {
        ctx.state = STATE::TRY_CATCH_PARAM;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '(' for catch parameter: " + std::string(1, c));
    }
}

inline void handleStateTryCatchParam(ParserContext& ctx, char c) {
    if (c == ')') {
        // End of parameter
        ctx.state = STATE::TRY_CATCH_BODY_START;
    } else {
        // Parse parameter
        ctx.state = STATE::EXPRESSION_EXPECT_OPERAND;
        // Re-process this character
        ctx.index--;
    }
}

inline void handleStateTryCatchBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for catch body
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* tryNode = dynamic_cast<TryStatement*>(ctx.currentNode)) {
            if (tryNode->handler) {
                tryNode->handler->body = block;
                tryNode->handler->addChild(block);
            }
        }
        ctx.currentNode = block;
        ctx.state = STATE::TRY_CATCH_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for catch body: " + std::string(1, c));
    }
}

inline void handleStateTryCatchBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of catch body
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::TRY_FINALLY_START;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip body content
        return;
    }
}

inline void handleStateTryFinallyStart(ParserContext& ctx, char c) {
    if (c == 'f') {
        // Finally clause
        ctx.state = STATE::TRY_FINALLY;
        // Re-process this character
        ctx.index--;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // No finally, end try statement
        ctx.currentNode = ctx.currentNode->parent;
        ctx.state = STATE::NONE;
    }
}

inline void handleStateTryFinally(ParserContext& ctx, char c) {
    if (c == 'i') {
        // Continue with "finally"
        return;
    } else if (c == 'n') {
        // Continue with "finally"
        return;
    } else if (c == 'a') {
        // Continue with "finally"
        return;
    } else if (c == 'l') {
        // Continue with "finally"
        return;
    } else if (c == 'y') {
        // Continue with "finally"
        return;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected 'finally' keyword: " + std::string(1, c));
    }
}

inline void handleStateTryFinallyBodyStart(ParserContext& ctx, char c) {
    if (c == '{') {
        // Block statement for finally body
        auto* block = new BlockStatement(ctx.currentNode);
        if (auto* tryNode = dynamic_cast<TryStatement*>(ctx.currentNode)) {
            if (tryNode->finalizer) {
                tryNode->finalizer->body = block;
                tryNode->finalizer->addChild(block);
            }
        }
        ctx.currentNode = block;
        ctx.state = STATE::TRY_FINALLY_BODY;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        throw std::runtime_error("Expected '{' for finally body: " + std::string(1, c));
    }
}

inline void handleStateTryFinallyBody(ParserContext& ctx, char c) {
    if (c == '}') {
        // End of finally body and try statement
        ctx.currentNode = ctx.currentNode->parent->parent;
        ctx.state = STATE::NONE;
    } else if (std::isspace(static_cast<unsigned char>(c))) {
        // Skip whitespace
        return;
    } else {
        // For now, skip body content
        return;
    }
}
