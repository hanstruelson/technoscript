#pragma once
#include "parser/lib/ast.h"
#include <iostream>
#include <stack>
#include <unordered_map>
#include <vector>

// Information about a variable definition
struct VariableInfo {
    std::string name;
    VariableDefinitionType varType;
    LexicalScopeNode* definingScope;
    int scopeDepth;
    bool isDefined; // For hoisting - true if actually defined, false if just declared
};

// Information about an unknown variable reference
struct UnknownVariableInfo {
    std::string name;
    ASTNode* referenceNode;
    int scopeDepth;
    LexicalScopeNode* scope;
};

// Analyzer context that maintains scope information
class AnalyzerContext {
public:
    std::stack<LexicalScopeNode*> scopeStack;
    std::unordered_map<std::string, VariableInfo> activeVariables; // identifier -> info
    std::unordered_map<std::string, std::vector<UnknownVariableInfo>> unknownVariables; // identifier -> list of references
    int currentScopeDepth;

    AnalyzerContext() : currentScopeDepth(1) {}

    void pushScope(LexicalScopeNode* scope) {
        scopeStack.push(scope);
        currentScopeDepth++;
    }

    void popScope() {
        if (!scopeStack.empty()) {
            LexicalScopeNode* poppedScope = scopeStack.top();
            scopeStack.pop();
            currentScopeDepth--;

            // Remove variables defined in this scope from activeVariables
            // and check for unknown variables that might now be resolved
            std::vector<std::string> toRemove;
            for (auto& pair : activeVariables) {
                if (pair.second.definingScope == poppedScope) {
                    toRemove.push_back(pair.first);
                }
            }

            // Check if any popped variables resolve unknown references
            for (const std::string& varName : toRemove) {
                auto it = unknownVariables.find(varName);
                if (it != unknownVariables.end()) {
                    // Check if any unknown references can be resolved
                    std::vector<UnknownVariableInfo> stillUnknown;
                    for (const auto& unknown : it->second) {
                        bool canResolve = false;
                        if (unknown.scopeDepth >= activeVariables[varName].scopeDepth) {
                            // Check if the defining scope is an ancestor of the unknown reference's scope
                            LexicalScopeNode* checkScope = unknown.scope;
                            while (checkScope != nullptr) {
                                if (checkScope == activeVariables[varName].definingScope) {
                                    canResolve = true;
                                    break;
                                }
                                // Find parent scope by walking up the AST
                                ASTNode* parent = checkScope->parent;
                                while (parent && dynamic_cast<LexicalScopeNode*>(parent) == nullptr) {
                                    parent = parent->parent;
                                }
                                checkScope = dynamic_cast<LexicalScopeNode*>(parent);
                            }
                        }

                        if (canResolve) {
                            // This reference can be resolved
                            std::cout << "Resolved unknown variable '" << varName
                                    << "' at depth " << unknown.scopeDepth
                                    << " to definition at depth " << activeVariables[varName].scopeDepth << std::endl;
                        } else {
                            stillUnknown.push_back(unknown);
                        }
                    }
                    if (stillUnknown.empty()) {
                        unknownVariables.erase(it);
                    } else {
                        it->second = stillUnknown;
                    }
                }
                activeVariables.erase(varName);
            }
        }
    }

    LexicalScopeNode* currentScope() {
        return scopeStack.empty() ? nullptr : scopeStack.top();
    }

    void defineVariable(const std::string& name, VariableDefinitionType varType, LexicalScopeNode* scope) {
        VariableInfo info{name, varType, scope, currentScopeDepth, true};
        activeVariables[name] = info;

        // Check if this resolves any unknown references
        auto it = unknownVariables.find(name);
        if (it != unknownVariables.end()) {
            std::vector<UnknownVariableInfo> stillUnknown;
            for (const auto& unknown : it->second) {
                // Only resolve if the unknown reference is in a descendant scope of the defining scope
                bool canResolve = false;
                if (unknown.scopeDepth > currentScopeDepth) {  // Must be deeper in the hierarchy
                    // Check if the defining scope is an ancestor of the unknown reference's scope
                    // Walk up from the unknown scope to see if we reach the defining scope
                    LexicalScopeNode* checkScope = unknown.scope;
                    while (checkScope != nullptr) {
                        if (checkScope == scope) {
                            canResolve = true;
                            break;
                        }
                        // Find parent scope by walking up the AST
                        ASTNode* parent = checkScope->parent;
                        while (parent && dynamic_cast<LexicalScopeNode*>(parent) == nullptr) {
                            parent = parent->parent;
                        }
                        checkScope = dynamic_cast<LexicalScopeNode*>(parent);
                    }
                }

                if (canResolve) {
                    // This reference can be resolved
                    std::cout << "Resolved unknown variable '" << name
                            << "' at depth " << unknown.scopeDepth
                            << " to definition at depth " << currentScopeDepth << std::endl;
                } else {
                    stillUnknown.push_back(unknown);
                }
            }
            if (stillUnknown.empty()) {
                unknownVariables.erase(it);
            } else {
                it->second = stillUnknown;
            }
        }
    }

    VariableInfo* findVariable(const std::string& name) {
        auto it = activeVariables.find(name);
        if (it != activeVariables.end()) {
            return &it->second;
        }
        return nullptr;
    }

    void addUnknownVariable(const std::string& name, ASTNode* node, LexicalScopeNode* scope) {
        UnknownVariableInfo info{name, node, currentScopeDepth, scope};
        unknownVariables[name].push_back(info);
        std::cout << "Added unknown variable '" << name << "' at depth " << currentScopeDepth << std::endl;
    }
};

// Analyzer class that walks the AST with scoping
class Analyzer {
private:
    AnalyzerContext context;

    void enterScope(LexicalScopeNode* scope) {
        context.pushScope(scope);
        std::cout << "Entered scope at depth " << context.currentScopeDepth << std::endl;
    }

    void exitScope() {
        context.popScope();
        std::cout << "Exited scope, now at depth " << context.currentScopeDepth << std::endl;
    }

    void visitVariableDefinition(VariableDefinitionNode* node) {
        if (!node->name.empty()) {
            context.defineVariable(node->name, node->varType, context.currentScope());
            std::cout << "Defined variable '" << node->name << "' of type "
                    << (node->varType == VariableDefinitionType::VAR ? "var" :
                        node->varType == VariableDefinitionType::LET ? "let" : "const")
                    << " at depth " << context.currentScopeDepth << std::endl;
        }
    }

    void visitIdentifierExpression(IdentifierExpressionNode* node) {
        VariableInfo* varInfo = context.findVariable(node->name);
        if (varInfo) {
            std::cout << "Found variable '" << node->name << "' defined at depth "
                    << varInfo->scopeDepth << " (current depth: " << context.currentScopeDepth << ")" << std::endl;
        } else {
            // Variable not found, add to unknown
            context.addUnknownVariable(node->name, node, context.currentScope());
        }
    }

    void visitNode(ASTNode* node) {
        if (!node) return;

        switch (node->nodeType) {
            case ASTNodeType::FUNCTION_DECLARATION: {
                auto* funcNode = static_cast<FunctionDeclarationNode*>(node);
                // Function name is defined in parent scope
                if (!funcNode->name.empty()) {
                    context.defineVariable(funcNode->name, VariableDefinitionType::VAR, context.currentScope());
                    std::cout << "Defined function '" << funcNode->name << "' at depth " << context.currentScopeDepth << std::endl;
                }
                // Enter function scope
                enterScope(funcNode);
                // Visit function body explicitly
                if (funcNode->body) {
                    visitNode(funcNode->body);
                }
                // Exit scope
                exitScope();
                return; // Don't walk children again
            }
            case ASTNodeType::BLOCK_STATEMENT: {
                auto* blockNode = static_cast<BlockStatement*>(node);
                enterScope(blockNode);
                // Walk children
                for (auto child : node->children) {
                    visitNode(child);
                }
                // Exit scope
                exitScope();
                return; // Don't walk children again
            }
            case ASTNodeType::VARIABLE_DEFINITION: {
                visitVariableDefinition(static_cast<VariableDefinitionNode*>(node));
                break;
            }
            case ASTNodeType::IDENTIFIER_EXPRESSION: {
                visitIdentifierExpression(static_cast<IdentifierExpressionNode*>(node));
                break;
            }
            default:
                break;
        }

        // Walk children for non-scope nodes
        for (auto child : node->children) {
            visitNode(child);
        }
    }

public:
    void analyze(ASTNode* root) {
        if (root) {
            // Start with root scope
            context.pushScope(nullptr); // Root scope
            visitNode(root);
            context.popScope();
        }

        // Report any remaining unknown variables
        if (!context.unknownVariables.empty()) {
            std::cout << "\nRemaining unknown variables:" << std::endl;
            for (const auto& pair : context.unknownVariables) {
                std::cout << "  '" << pair.first << "': " << pair.second.size() << " references" << std::endl;
            }
        }
    }
};

// Test function to demonstrate analyzer
void testAnalyzer() {
    std::cout << "=== Test 1: Basic scoping ===" << std::endl;
    {
        // Create a simple AST for testing
        auto* root = new ASTNode(nullptr);

        // Create a function declaration
        auto* func = new FunctionDeclarationNode(root);
        func->name = "testFunction";

        // Create a block statement inside the function
        auto* block = new BlockStatement(func);
        func->body = block;

        // Add an identifier reference BEFORE definition (should be unknown initially)
        auto* ident1 = new IdentifierExpressionNode(block, "x");
        block->addChild(ident1);

        // Add a variable definition in the block
        auto* varDef = new VariableDefinitionNode(block, VariableDefinitionType::LET);
        varDef->name = "x";
        block->addChild(varDef);

        // Add another identifier reference AFTER definition
        auto* ident2 = new IdentifierExpressionNode(block, "x");
        block->addChild(ident2);

        // Add reference to undefined variable
        auto* ident3 = new IdentifierExpressionNode(block, "undefinedVar");
        block->addChild(ident3);

        // Add the function to root
        root->addChild(func);

        // Analyze the AST
        Analyzer analyzer;
        std::cout << "Starting AST analysis:" << std::endl;
        analyzer.analyze(root);

        // Clean up
        delete root;
    }

    std::cout << "\n=== Test 2: Hoisting with var ===" << std::endl;
    {
        // Test hoisting behavior with var
        auto* root = new ASTNode(nullptr);

        auto* func = new FunctionDeclarationNode(root);
        func->name = "testHoisting";

        auto* block = new BlockStatement(func);
        func->body = block;

        // Reference before definition (should work with var due to hoisting)
        auto* ident1 = new IdentifierExpressionNode(block, "hoistedVar");
        block->addChild(ident1);

        // Define with var (should hoist)
        auto* varDef = new VariableDefinitionNode(block, VariableDefinitionType::VAR);
        varDef->name = "hoistedVar";
        block->addChild(varDef);

        root->addChild(func);

        Analyzer analyzer;
        analyzer.analyze(root);

        delete root;
    }

    std::cout << "\n=== Test 3: Nested scopes ===" << std::endl;
    {
        // Test nested scopes
        auto* root = new ASTNode(nullptr);

        auto* func = new FunctionDeclarationNode(root);
        func->name = "testNested";

        auto* outerBlock = new BlockStatement(func);
        func->body = outerBlock;

        // Variable in outer scope
        auto* outerVar = new VariableDefinitionNode(outerBlock, VariableDefinitionType::LET);
        outerVar->name = "outerVar";
        outerBlock->addChild(outerVar);

        // Nested block
        auto* innerBlock = new BlockStatement(outerBlock);
        outerBlock->addChild(innerBlock);

        // Reference outer variable from inner scope
        auto* innerRef = new IdentifierExpressionNode(innerBlock, "outerVar");
        innerBlock->addChild(innerRef);

        // Define variable with same name in inner scope (shadowing)
        auto* innerVar = new VariableDefinitionNode(innerBlock, VariableDefinitionType::LET);
        innerVar->name = "outerVar"; // Same name
        innerBlock->addChild(innerVar);

        // Reference should now find inner variable
        auto* innerRef2 = new IdentifierExpressionNode(innerBlock, "outerVar");
        innerBlock->addChild(innerRef2);

        root->addChild(func);

        Analyzer analyzer;
        analyzer.analyze(root);

        delete root;
    }

    std::cout << "\n=== Test 4: Unrelated scopes (should not correlate) ===" << std::endl;
    {
        // Test the user's specific case: y in test2 should not correlate with y in test3
        auto* root = new ASTNode(nullptr);

        // function test() { function test2() { console.log(y) } }
        auto* testFunc = new FunctionDeclarationNode(root);
        testFunc->name = "test";

        auto* testBlock = new BlockStatement(testFunc);
        testFunc->body = testBlock;

        auto* test2Func = new FunctionDeclarationNode(testBlock);
        test2Func->name = "test2";

        auto* test2Block = new BlockStatement(test2Func);
        test2Func->body = test2Block;

        // Reference to y in test2
        auto* yRef = new IdentifierExpressionNode(test2Block, "y");
        test2Block->addChild(yRef);

        testBlock->addChild(test2Func); // Add test2 to test's body
        root->addChild(testFunc);

        // function test3() { var y = 5 }
        auto* test3Func = new FunctionDeclarationNode(root);
        test3Func->name = "test3";

        auto* test3Block = new BlockStatement(test3Func);
        test3Func->body = test3Block;

        // Define y in test3
        auto* yDef = new VariableDefinitionNode(test3Block, VariableDefinitionType::VAR);
        yDef->name = "y";
        test3Block->addChild(yDef);

        root->addChild(test3Func);

        Analyzer analyzer;
        analyzer.analyze(root);

        delete root;
    }
}
