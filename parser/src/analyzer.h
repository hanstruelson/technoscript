#pragma once
#include "parser/lib/ast.h"
#include <iostream>
#include <stack>
#include <unordered_map>
#include <vector>
#include <map>

// Information about an unknown variable reference
struct UnknownVariableInfo {
    std::string name;
    ASTNode* referenceNode;
    int scopeDepth;
    LexicalScopeNode* scope;
};

// Scope layout constants
namespace ScopeLayout {
    constexpr int DATA_OFFSET = 16;
}

// Shared packing utility for both lexical scopes and classes
namespace VariablePacking {
    // Get size for a type (without closure/object special handling)
    inline int getBaseTypeSize(DataType type) {
        switch (type) {
            case DataType::INT64: return 8;
            case DataType::FLOAT64: return 8;
            case DataType::STRING: return 8;
            case DataType::RAW_MEMORY: return 8;
            case DataType::OBJECT: return 8;
            default: return 8;
        }
    }

    // Pack a collection of variables, returns total size
    inline int packVariables(std::vector<VariableInfo*>& vars) {
        // Sort by size (biggest first) for optimal packing
        std::sort(vars.begin(), vars.end(), [](const VariableInfo* a, const VariableInfo* b) {
            return a->size > b->size;
        });

        int offset = ScopeLayout::DATA_OFFSET;
        for (auto* var : vars) {
            int size = var->size;
            int align = (var->type == DataType::OBJECT) ? 8 : size;
            offset = (offset + align - 1) & ~(align - 1); // Align
            var->offset = offset;
            offset += size;
        }

        // Return 8-byte aligned total size
        return (offset + 7) & ~7;
    }
};

// Analyzer context that maintains scope information
class AnalyzerContext {
public:
    std::stack<LexicalScopeNode*> scopeStack;
    std::unordered_map<std::string, VariableInfo> activeVariables; // identifier -> info
    std::unordered_map<std::string, std::vector<UnknownVariableInfo>> unknownVariables; // identifier -> list of references
    std::map<std::string, ClassDeclarationNode*> classRegistry; // class name -> class
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
        VariableInfo info;
        info.name = name;
        info.varType = varType;
        info.definingScope = scope;
        info.scopeDepth = currentScopeDepth;
        info.isDefined = true;
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

    ClassDeclarationNode* findClass(const std::string& className) {
        auto it = classRegistry.find(className);
        if (it == classRegistry.end()) {
            throw std::runtime_error("Class '" + className + "' not found");
        }
        return it->second;
    }

    void addClass(ClassDeclarationNode* classDecl) {
        classRegistry[classDecl->name] = classDecl;
    }
};

// Analyzer class that performs comprehensive analysis like the old analyzer
class Analyzer {
private:
    AnalyzerContext context;
    FunctionDeclarationNode* currentMethodContext = nullptr; // Track which method we're currently analyzing
    ClassDeclarationNode* currentClassContext = nullptr;     // Track which class the current method belongs to

    // Single-pass analysis method that does everything efficiently
    void analyzeNodeSinglePass(ASTNode* node, LexicalScopeNode* parentScope, int depth) {
        if (!node) return;

        // Set depth on lexical scopes
        if (auto* scopeNode = dynamic_cast<LexicalScopeNode*>(node)) {
            scopeNode->depth = depth;
            scopeNode->parentFunctionScope = parentScope;
        }

        switch (node->nodeType) {
            case ASTNodeType::FUNCTION_DECLARATION: {
                auto* funcNode = static_cast<FunctionDeclarationNode*>(node);

                // Define function name in parent scope
                if (!funcNode->name.empty() && parentScope) {
                    VariableInfo funcInfo;
                    funcInfo.name = funcNode->name;
                    funcInfo.type = DataType::OBJECT; // Functions are objects
                    funcInfo.size = 8;
                    funcInfo.funcNode = funcNode;
                    parentScope->variables[funcNode->name] = funcInfo;
                }

                // Collect var declarations for hoisting to function scope
                if (funcNode->body) {
                    collectVarDeclarations(funcNode->body, funcNode);
                }

                // Analyze function body with function as parent scope
                if (funcNode->body) {
                    analyzeNodeSinglePass(funcNode->body, funcNode, depth + 1);
                }

                // Calculate parameter layout
                if (funcNode->parameters) {
                    int paramOffset = ScopeLayout::DATA_OFFSET; // Skip return address and saved RBP
                    for (auto* param : funcNode->parameters->parameters) {
                        if (param && !param->pattern->value.empty()) {
                            VariableInfo paramInfo;
                            paramInfo.name = param->pattern->value;
                            paramInfo.type = DataType::INT64; // Default type
                            paramInfo.size = 8;
                            paramInfo.offset = paramOffset;
                            funcNode->variables[param->pattern->value] = paramInfo;
                            paramOffset += 8;
                        }
                    }
                }

                // Pack function scope variables
                std::vector<VariableInfo*> funcVars;
                for (auto& varPair : funcNode->variables) {
                    funcVars.push_back(&varPair.second);
                }
                funcNode->totalSize = VariablePacking::packVariables(funcVars);

                break;
            }

            case ASTNodeType::BLOCK_STATEMENT: {
                auto* blockNode = static_cast<BlockStatement*>(node);

                // Analyze children with block as parent scope
                for (auto* child : node->children) {
                    analyzeNodeSinglePass(child, blockNode, depth + 1);
                }

                // Pack block scope variables
                std::vector<VariableInfo*> blockVars;
                for (auto& varPair : blockNode->variables) {
                    blockVars.push_back(&varPair.second);
                }
                blockNode->totalSize = VariablePacking::packVariables(blockVars);

                break;
            }

            case ASTNodeType::VARIABLE_DEFINITION: {
                auto* varDef = static_cast<VariableDefinitionNode*>(node);

                // Only add let/const to current scope - var is already hoisted to function scope
                if (varDef->varType != VariableDefinitionType::VAR && !varDef->name.empty() && parentScope) {
                    VariableInfo varInfo;
                    varInfo.name = varDef->name;
                    varInfo.varType = varDef->varType;
                    varInfo.type = DataType::INT64; // Default type
                    varInfo.size = 8;
                    varInfo.definingScope = parentScope;

                    parentScope->variables[varDef->name] = varInfo;
                }

                // Analyze initializer
                if (varDef->initializer) {
                    analyzeNodeSinglePass(varDef->initializer, parentScope, depth);
                }

                break;
            }

            case ASTNodeType::IDENTIFIER_EXPRESSION: {
                auto* identNode = static_cast<IdentifierExpressionNode*>(node);

                // Resolve variable reference
                VariableInfo* varInfo = findVariable(identNode->name, parentScope);
                if (varInfo) {
                    identNode->varRef = varInfo;
                    identNode->accessedIn = parentScope;

                    // Track dependencies for closure analysis
                    if (varInfo->definingScope != parentScope) {
                        addParentDep(parentScope, varInfo->definingScope->depth);
                        addDescendantDep(varInfo->definingScope, parentScope->depth);
                    }
                } else {
                    // Unresolved variable - this might be a global or error
                    std::cout << "Warning: Unresolved identifier '" << identNode->name << "' at depth " << depth << std::endl;
                }

                break;
            }

            case ASTNodeType::MEMBER_ACCESS: {
                auto* memberAccess = static_cast<MemberAccessNode*>(node);

                // Analyze object expression
                if (memberAccess->object) {
                    analyzeNodeSinglePass(memberAccess->object, parentScope, depth);
                }

                // Resolve member access if object is a known class instance
                // This is simplified - in a real implementation we'd need type inference
                if (memberAccess->object && memberAccess->object->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                    auto* ident = static_cast<IdentifierExpressionNode*>(memberAccess->object);
                    if (ident->varRef && ident->varRef->classNode) {
                        memberAccess->classRef = ident->varRef->classNode;

                        // Find field offset
                        auto it = ident->varRef->classNode->fields.find(memberAccess->memberName);
                        if (it != ident->varRef->classNode->fields.end()) {
                            memberAccess->memberOffset = it->second.offset;
                        }
                    }
                }

                break;
            }

            case ASTNodeType::METHOD_CALL: {
                auto* methodCall = static_cast<MethodCallNode*>(node);

                // Analyze object expression
                if (methodCall->object) {
                    analyzeNodeSinglePass(methodCall->object, parentScope, depth);
                }

                // Analyze arguments
                for (auto* arg : methodCall->args) {
                    analyzeNodeSinglePass(arg, parentScope, depth);
                }

                // Resolve method if object is a known class instance
                if (methodCall->object && methodCall->object->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                    auto* ident = static_cast<IdentifierExpressionNode*>(methodCall->object);
                    if (ident->varRef && ident->varRef->classNode) {
                        methodCall->objectClass = ident->varRef->classNode;

                        // Find method in class
                        auto* methodInfo = findMethodInClass(ident->varRef->classNode, methodCall->methodName);
                        if (methodInfo) {
                            methodCall->resolvedMethod = methodInfo->method;
                            // Calculate index in method layout array
                            methodCall->methodLayoutIndex = methodInfo - &ident->varRef->classNode->methodLayout[0];
                            methodCall->thisOffset = methodInfo->thisOffset;
                            methodCall->methodClosureOffset = methodInfo->closureOffsetInObject;
                        }
                    }
                }

                break;
            }

            case ASTNodeType::THIS_EXPR: {
                auto* thisExpr = static_cast<ThisExprNode*>(node);

                // Set class and method context
                thisExpr->classContext = currentClassContext;
                thisExpr->methodContext = currentMethodContext;

                break;
            }

            case ASTNodeType::NEW_EXPR: {
                auto* newExpr = static_cast<NewExprNode*>(node);

                // Resolve class reference
                try {
                    newExpr->classRef = context.findClass(newExpr->className);
                } catch (const std::runtime_error&) {
                    std::cout << "Warning: Class '" << newExpr->className << "' not found for new expression" << std::endl;
                }

                // Analyze constructor arguments
                for (auto* arg : newExpr->args) {
                    analyzeNodeSinglePass(arg, parentScope, depth);
                }

                break;
            }

            case ASTNodeType::CLASS_DECLARATION: {
                auto* classDecl = static_cast<ClassDeclarationNode*>(node);

                // Set current class context for method analysis
                ClassDeclarationNode* prevClassContext = currentClassContext;
                currentClassContext = classDecl;

                // Analyze class members
                for (auto* child : node->children) {
                    if (child->nodeType == ASTNodeType::CLASS_METHOD) {
                        auto* method = static_cast<ClassMethodNode*>(child);

                        // Set current method context
                        FunctionDeclarationNode* prevMethodContext = currentMethodContext;
                        currentMethodContext = static_cast<FunctionDeclarationNode*>(method->body);

                        // Analyze method body
                        if (method->body) {
                            analyzeNodeSinglePass(method->body, classDecl, depth + 1);
                        }

                        currentMethodContext = prevMethodContext;
                    } else {
                        analyzeNodeSinglePass(child, parentScope, depth);
                    }
                }

                currentClassContext = prevClassContext;
                break;
            }

            default: {
                // For all other node types, just recursively analyze children
                for (auto* child : node->children) {
                    analyzeNodeSinglePass(child, parentScope, depth);
                }
                break;
            }
        }
    }

    // Variable resolution (still needed)
    VariableInfo* findVariable(const std::string& name, LexicalScopeNode* scope) {
        if (!scope) return nullptr;

        // Check current scope
        auto it = scope->variables.find(name);
        if (it != scope->variables.end()) {
            return &it->second;
        }

        // Check parent scopes recursively
        LexicalScopeNode* parent = scope->parentFunctionScope;
        while (parent) {
            auto parentIt = parent->variables.find(name);
            if (parentIt != parent->variables.end()) {
                return &parentIt->second;
            }
            parent = parent->parentFunctionScope;
        }

        return nullptr;
    }

    // Class inheritance helpers (called once per class during first pass)
    void resolveClassInheritance(ClassDeclarationNode* classDecl) {
        if (!classDecl->extendsClass.empty()) {
            try {
                ClassDeclarationNode* parentClass = context.findClass(classDecl->extendsClass);
                classDecl->parentRefs.push_back(parentClass);
                classDecl->parentClassNames.push_back(classDecl->extendsClass);

                // Calculate parent offset (simplified - just accumulate sizes)
                int offset = 0;
                for (auto* parent : classDecl->parentRefs) {
                    classDecl->parentOffsets[parent->name] = offset;
                    offset += parent->totalSize;
                }
            } catch (const std::runtime_error&) {
                std::cerr << "Warning: Parent class '" << classDecl->extendsClass << "' not found for class '" << classDecl->name << "'" << std::endl;
            }
        }
    }

    void calculateClassLayout(ClassDeclarationNode* classDecl) {
        std::vector<VariableInfo*> allFields;

        // Collect fields from all parent classes first
        for (auto* parent : classDecl->parentRefs) {
            for (auto& fieldPair : parent->fields) {
                allFields.push_back(&fieldPair.second);
                classDecl->allFieldsInOrder.push_back(fieldPair.first);
            }
        }

        // Add own fields
        for (auto* prop : classDecl->properties) {
            if (prop->name.empty()) continue;

            VariableInfo fieldInfo;
            fieldInfo.name = prop->name;
            fieldInfo.type = DataType::OBJECT; // Default to object for now
            fieldInfo.size = 8; // Default size
            fieldInfo.classNode = classDecl;

            classDecl->fields[prop->name] = fieldInfo;
            allFields.push_back(&classDecl->fields[prop->name]);
            classDecl->allFieldsInOrder.push_back(prop->name);
        }

        // Pack all fields
        classDecl->totalSize = VariablePacking::packVariables(allFields);
    }

    void buildClassVTable(ClassDeclarationNode* classDecl) {
        // Collect all methods from inheritance hierarchy
        std::map<std::string, ClassDeclarationNode*> methodDefiningClasses;

        // Start with own methods
        for (auto* method : classDecl->methods) {
            if (method->name.empty()) continue;
            methodDefiningClasses[method->name] = classDecl;
        }

        // Add inherited methods (if not overridden)
        for (auto* parent : classDecl->parentRefs) {
            for (auto& methodInfo : parent->methodLayout) {
                if (methodDefiningClasses.find(methodInfo.methodName) == methodDefiningClasses.end()) {
                    methodDefiningClasses[methodInfo.methodName] = methodInfo.definingClass;
                }
            }
        }

        // Build method layout
        int methodIndex = 0;
        for (auto& pair : methodDefiningClasses) {
            const std::string& methodName = pair.first;
            ClassDeclarationNode* definingClass = pair.second;

            // Find the actual method node
            FunctionDeclarationNode* methodNode = nullptr;
            if (definingClass == classDecl) {
                // Own method
                for (auto* method : classDecl->methods) {
                    if (method->name == methodName) {
                        methodNode = static_cast<FunctionDeclarationNode*>(method->body); // Assuming method body is a function
                        break;
                    }
                }
            } else {
                // Inherited method - find in parent's layout
                for (auto& parentMethod : definingClass->methodLayout) {
                    if (parentMethod.methodName == methodName) {
                        methodNode = parentMethod.method;
                        break;
                    }
                }
            }

            if (methodNode) {
                ClassDeclarationNode::MethodLayoutInfo layoutInfo;
                layoutInfo.methodName = methodName;
                layoutInfo.method = methodNode;
                layoutInfo.definingClass = definingClass;

                // Calculate this offset adjustment for inherited methods
                if (definingClass != classDecl) {
                    auto it = classDecl->parentOffsets.find(definingClass->name);
                    if (it != classDecl->parentOffsets.end()) {
                        layoutInfo.thisOffset = it->second;
                    }
                }

                // Calculate closure size and offset
                layoutInfo.closureSize = 8; // Simplified - assume 8 bytes for closure
                layoutInfo.closureOffsetInObject = classDecl->totalSize + (methodIndex * 8);

                // Create virtual field for closure pointer
                layoutInfo.closurePointerField.name = "__method_" + methodName + "_closure";
                layoutInfo.closurePointerField.type = DataType::OBJECT;
                layoutInfo.closurePointerField.size = 8;
                layoutInfo.closurePointerField.offset = layoutInfo.closureOffsetInObject;

                classDecl->methodLayout.push_back(layoutInfo);
                methodIndex++;
            }
        }
    }

    // Method resolution for method calls
    ClassDeclarationNode::MethodLayoutInfo* findMethodInClass(ClassDeclarationNode* classDecl, const std::string& methodName) {
        for (auto& methodInfo : classDecl->methodLayout) {
            if (methodInfo.methodName == methodName) {
                return &methodInfo;
            }
        }
        return nullptr;
    }

    // Collect var declarations for hoisting
    void collectVarDeclarations(ASTNode* node, LexicalScopeNode* targetScope) {
        if (!node) return;

        if (node->nodeType == ASTNodeType::VARIABLE_DEFINITION) {
            auto* varDef = static_cast<VariableDefinitionNode*>(node);
            if (varDef->varType == VariableDefinitionType::VAR && !varDef->name.empty()) {
                VariableInfo varInfo;
                varInfo.name = varDef->name;
                varInfo.varType = varDef->varType;
                varInfo.type = DataType::INT64;
                varInfo.size = 8;
                varInfo.definingScope = targetScope;
                targetScope->variables[varDef->name] = varInfo;
            }
        } else if (node->nodeType == ASTNodeType::FUNCTION_DECLARATION) {
            // Skip nested functions - they handle their own vars
            return;
        }

        for (auto* child : node->children) {
            collectVarDeclarations(child, targetScope);
        }
    }

    // Helper methods for dependency tracking
    void addParentDep(LexicalScopeNode* scope, int depthIdx) {
        if (scope && scope->parentDeps.find(depthIdx) == scope->parentDeps.end()) {
            scope->parentDeps.insert(depthIdx);
            scope->updateAllNeeded();
        }
    }

    void addDescendantDep(LexicalScopeNode* scope, int depthIdx) {
        if (scope && scope->descendantDeps.find(depthIdx) == scope->descendantDeps.end()) {
            scope->descendantDeps.insert(depthIdx);
            scope->updateAllNeeded();
        }
    }



public:
    void analyze(ASTNode* root) {
        if (!root) return;

        this->root = root; // Store root for registry access

        // First pass: collect all class declarations and resolve inheritance
        collectClassesAndResolveInheritance(root);

        // Second pass: build class layouts and method tables
        buildClassLayoutsAndMethods();

        // Third pass: analyze all expressions and resolve variables/methods
        analyzeNodeSinglePass(root, nullptr, 0);
    }

    // Getters for registries
    const std::map<std::string, ClassDeclarationNode*>& getClassRegistry() const {
        return context.classRegistry;
    }

    std::vector<FunctionDeclarationNode*> getFunctionRegistry() {
        std::vector<FunctionDeclarationNode*> functions;
        collectFunctionsFromAST(root, functions);
        return functions;
    }

private:
    // First pass: collect classes and resolve inheritance
    void collectClassesAndResolveInheritance(ASTNode* node) {
        if (!node) return;

        if (node->nodeType == ASTNodeType::CLASS_DECLARATION) {
            auto* classDecl = static_cast<ClassDeclarationNode*>(node);
            context.addClass(classDecl);
            resolveClassInheritance(classDecl);
        }

        for (auto* child : node->children) {
            collectClassesAndResolveInheritance(child);
        }
    }

    // Second pass: build layouts and method tables
    void buildClassLayoutsAndMethods() {
        for (auto& pair : context.classRegistry) {
            calculateClassLayout(pair.second);
            buildClassVTable(pair.second);
        }
    }

private:
    ASTNode* root; // Store the root for function collection

    void collectFunctionsFromAST(ASTNode* node, std::vector<FunctionDeclarationNode*>& functions) {
        if (!node) return;

        if (node->nodeType == ASTNodeType::FUNCTION_DECLARATION) {
            functions.push_back(static_cast<FunctionDeclarationNode*>(node));
        }

        for (auto* child : node->children) {
            collectFunctionsFromAST(child, functions);
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
