#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>

// Include asmjit for Label type
#include <asmjit/asmjit.h>

// Include the new parser's AST definitions
#include "parser/src/parser/lib/ast.h"

// Forward declarations for types used in compatibility classes
class ScopeMetadata;

// Robustness limits to prevent infinite loops and hangs
namespace RobustnessLimits {
    constexpr int MAX_SCOPE_TRAVERSAL_DEPTH = 50;
    constexpr int MAX_AST_RECURSION_DEPTH = 1000;
    constexpr int MAX_PARSER_ITERATIONS = 10000;
    constexpr int MAX_ANALYSIS_ITERATIONS = 10000;
}

// Type aliases for backward compatibility - use new parser's types directly
using AstNodeType = ASTNodeType;
using DataType = ::DataType;
using ParameterInfo = ::ParameterInfo;
using VariableInfo = ::VariableInfo;
using ClosurePatchInfo = ::ClosurePatchInfo;
using FunctionDeclNode = ::FunctionDeclarationNode;
using LexicalScopeNode = ::LexicalScopeNode;
using ClassDeclNode = ::ClassDeclarationNode;
using IdentifierNode = ::IdentifierExpressionNode;
using LiteralNode = ::LiteralExpressionNode;
using FunctionCallNode = ::MethodCallNode; // Note: MethodCallNode is more general
using BinaryExprNode = ::BinaryExpressionNode;
using BlockStmtNode = ::BlockStatement;
using NewExprNode = ::NewExprNode;
using MemberAccessNode = ::MemberAccessNode;
using MemberAssignNode = ::MemberAssignNode;
using MethodCallNode = ::MethodCallNode;
using ThisNode = ::ThisExprNode;
using VariableDefinitionNode = ::VariableDefinitionNode;

// Legacy AST node types for compatibility
enum class LegacyAstNodeType {
    VAR_DECL, FUNCTION_DECL, FUNCTION_CALL,
    IDENTIFIER, LITERAL, PRINT_STMT, GO_STMT, SETTIMEOUT_STMT,
    AWAIT_EXPR, SLEEP_CALL, FOR_STMT, LET_DECL,
    BINARY_EXPR, UNARY_EXPR, BLOCK_STMT,
    CLASS_DECL, NEW_EXPR, MEMBER_ACCESS, MEMBER_ASSIGN,
    METHOD_CALL, THIS_EXPR,
    BRACKET_ACCESS
};

// Add compatibility members to old AST classes to work with new parser
// These are added to make the compiler work with the new AST natively
namespace ASTCompatibility {

// Add missing members to FunctionDeclarationNode
class FunctionDeclarationNodeCompat : public ::FunctionDeclarationNode {
public:
    // Add missing members that codegen expects
    asmjit::Label* asmjitLabel = nullptr;
    bool isMethod = false;
    ClassDeclNode* owningClass = nullptr;
    std::string funcName; // Alias for name

    FunctionDeclarationNodeCompat(ASTNode* parent) : ::FunctionDeclarationNode(parent) {
        funcName = this->name; // Sync with name
    }

    // Override name setter to keep funcName in sync
    void setName(const std::string& n) {
        this->name = n;
        funcName = n;
    }
};

// Add missing members to LexicalScopeNode
class LexicalScopeNodeCompat : public ::LexicalScopeNode {
public:
    ScopeMetadata* metadata = nullptr;
    std::map<int, int> scopeDepthToParentParameterIndexMap;

    LexicalScopeNodeCompat(ASTNode* parent, bool isBlockScope)
        : ::LexicalScopeNode(parent, isBlockScope) {}
};

// Add missing members to ClassDeclarationNode
class ClassDeclarationNodeCompat : public ::ClassDeclarationNode {
public:
    std::string className; // Alias for name

    ClassDeclarationNodeCompat(ASTNode* parent) : ::ClassDeclarationNode(parent) {
        className = this->name; // Sync with name
    }

    // Override name setter to keep className in sync
    void setName(const std::string& n) {
        this->name = n;
        className = n;
    }
};

// Add missing members to ASTNode
class ASTNodeCompat : public ::ASTNode {
public:
    LegacyAstNodeType type = LegacyAstNodeType::VAR_DECL; // Legacy type field

    ASTNodeCompat(ASTNode* parent) : ::ASTNode(parent) {}
};

// Add missing members to VariableDefinitionNode
class VariableDefinitionNodeCompat : public ::VariableDefinitionNode {
public:
    bool isArray = false;
    std::string varName; // Alias for name
    DataType varType = DataType::INT64;

    VariableDefinitionNodeCompat(ASTNode* parent, VariableDefinitionType vType)
        : ::VariableDefinitionNode(parent, vType) {
        varName = this->name;
        // Map VariableDefinitionType to DataType
        switch (vType) {
            case VariableDefinitionType::CONST:
            case VariableDefinitionType::VAR:
            case VariableDefinitionType::LET:
                varType = DataType::INT64; // Default
                break;
        }
    }
};

// Add missing members to IdentifierExpressionNode
class IdentifierExpressionNodeCompat : public ::IdentifierExpressionNode {
public:
    VariableInfo* varRef = nullptr;
    std::string value; // Alias for name

    IdentifierExpressionNodeCompat(ASTNode* parent, const std::string& identifier)
        : ::IdentifierExpressionNode(parent, identifier) {
        value = this->name;
    }

    // Add missing getVariableAccess method
    struct VariableAccess {
        bool inCurrentScope = false;
        int scopeParameterIndex = -1;
        int offset = 0;
    };

    VariableAccess getVariableAccess() {
        VariableAccess access;
        if (varRef) {
            access.inCurrentScope = (varRef->definedIn == nullptr); // Simplified
            access.offset = varRef->offset;
        }
        return access;
    }
};

// Add missing members to LiteralExpressionNode
class LiteralExpressionNodeCompat : public ::LiteralExpressionNode {
public:
    enum class LiteralType { STRING, NUMBER };
    LiteralType literalKind = LiteralType::NUMBER;

    LiteralExpressionNodeCompat(ASTNode* parent, const std::string& value)
        : ::LiteralExpressionNode(parent, value) {
        // Determine literal kind from value
        try {
            std::stod(value);
            literalKind = LiteralType::NUMBER;
        } catch (...) {
            literalKind = LiteralType::STRING;
        }
    }
};

// Add missing members to BlockStatement
class BlockStatementCompat : public ::BlockStatement {
public:
    ScopeMetadata* metadata = nullptr;

    BlockStatementCompat(ASTNode* parent, bool noBraces = false)
        : ::BlockStatement(parent, noBraces) {}
};

// Add missing members to MethodCallNode
class MethodCallNodeCompat : public ::MethodCallNode {
public:
    VariableInfo* varRef = nullptr;

    MethodCallNodeCompat(ASTNode* parent, const std::string& method)
        : ::MethodCallNode(parent, method) {}
};

} // namespace ASTCompatibility
