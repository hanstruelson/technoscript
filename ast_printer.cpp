#include "ast_printer.h"
#include <iostream>

void printAST(ASTNode* node, int indent) {
    std::string spaces(indent * 2, ' ');
    std::cout << spaces;

    // Simple AST printer - just show basic node types for now
    switch (node->nodeType) {
        case ASTNodeType::FUNCTION_DECLARATION: {
            auto func = static_cast<FunctionDeclarationNode*>(node);
            std::cout << "FUNCTION " << func->name;
            break;
        }
        case ASTNodeType::VARIABLE_DEFINITION: {
            auto var = static_cast<VariableDefinitionNode*>(node);
            std::cout << "VAR " << var->name;
            break;
        }
        case ASTNodeType::IDENTIFIER_EXPRESSION: {
            auto id = static_cast<IdentifierExpressionNode*>(node);
            std::cout << "ID " << id->name;
            break;
        }
        case ASTNodeType::LITERAL_EXPRESSION: {
            auto lit = static_cast<LiteralExpressionNode*>(node);
            std::cout << "LIT " << lit->literal;
            break;
        }
        case ASTNodeType::BLOCK_STATEMENT: {
            std::cout << "BLOCK";
            break;
        }
        case ASTNodeType::CLASS_DECLARATION: {
            auto cls = static_cast<ClassDeclarationNode*>(node);
            std::cout << "CLASS " << cls->name;
            break;
        }
        case ASTNodeType::MEMBER_ACCESS: {
            auto member = static_cast<MemberAccessNode*>(node);
            std::cout << "MEMBER ." << member->memberName;
            break;
        }
        case ASTNodeType::METHOD_CALL: {
            auto method = static_cast<MethodCallNode*>(node);
            std::cout << "METHOD ." << method->methodName;
            break;
        }
        case ASTNodeType::NEW_EXPR: {
            auto newExpr = static_cast<NewExprNode*>(node);
            std::cout << "NEW " << newExpr->className;
            break;
        }
        default:
            std::cout << "NODE type=" << static_cast<int>(node->nodeType);
            break;
    }
    std::cout << "\n";

    for (auto* child : node->children) {
        printAST(child, indent + 1);
    }
}
