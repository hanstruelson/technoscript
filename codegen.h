#pragma once

#include "parser/src/parser/lib/ast.h"
#include "gc.h"  // Must be before goroutine.h since goroutine uses GoroutineGCState
#include "library.h"
#include "goroutine.h"
#include "asm_library.h"
#include <asmjit/asmjit.h>
#include <capstone/capstone.h>
#include <memory>
#include <unordered_map>
#include <optional>

using namespace asmjit;

// Metadata structures forward declarations (defined in gc.h)
class ScopeMetadata;
class ClassMetadata;

// Object memory layout constants
namespace ObjectLayout {
    constexpr int METADATA_OFFSET = 0;
    constexpr int METADATA_SIZE = 8;

    constexpr int FLAGS_OFFSET = 8;
    constexpr int FLAGS_SIZE = 8;

    constexpr int HEADER_SIZE = 16;    // Metadata + flags
    // After header: method closures are stored (size varies by class)
    // Then fields start (offset calculated based on method closures size)
}

// Tensor slice layout constants
namespace SliceLayout {
    constexpr int NDIM_OFFSET = 0;    // Number of dimensions (qword)
    constexpr int NDIM_SIZE = 8;

    constexpr int DIM_START = 8;      // Start of dimension data
    constexpr int DIM_ENTRY_SIZE = 24; // start(8) + stop(8) + step(8)
    constexpr int START_OFFSET = 0;    // Relative to dimension entry
    constexpr int STOP_OFFSET = 8;     // Relative to dimension entry
    constexpr int STEP_OFFSET = 16;    // Relative to dimension entry

    // Calculate dimension entry offset (0-based index)
    constexpr int getDimOffset(int dimIndex) {
        return DIM_START + (dimIndex * DIM_ENTRY_SIZE);
    }
}

// Lexical Scope memory layout constants
namespace ScopeLayout {
    constexpr int FLAGS_OFFSET = 0;
    constexpr int FLAGS_SIZE = 8;
    constexpr int METADATA_OFFSET = 8;
    constexpr int METADATA_SIZE = 8;
    // DATA_OFFSET is defined in analyzer.h
}

class CodeGenerator {
public:
    CodeGenerator();
    ~CodeGenerator();
    
    // Main code generation entry point
    void* generateCode(ASTNode* root, const std::map<std::string, ClassDeclarationNode*>& classRegistry);
    
    // Core codegen methods for the basic functionality
    void allocateScope(LexicalScopeNode* scope);
    void assignVariable(VariableDefinitionNode* varDecl, ASTNode* value);
    void printInt64(IdentifierExpressionNode* identifier);
    
    // Assembly disassembly and printing
    void disassembleAndPrint(void* code, size_t codeSize);
    
private:
    JitRuntime rt;
    CodeHolder code;
    x86::Builder* cb = nullptr; // Builder pointer - created fresh each time
    csh capstoneHandle;
    
    // Assembly library for built-in functions
    std::unique_ptr<AsmLibrary> asmLibrary;
    
    // Current function context
    LexicalScopeNode* currentScope;
    std::unordered_map<LexicalScopeNode*, x86::Gp> scopeRegisters;

    // Function labels map (since FunctionDeclarationNode doesn't have asmjitLabel member)
    std::unordered_map<FunctionDeclarationNode*, asmjit::Label> functionLabels;
    
    // Helper methods
    void visitNode(ASTNode* node);
    void generateProgram(ASTNode* root);
    void generateAllFunctions(const std::vector<FunctionDeclarationNode*>& functionRegistry);
    void generateVarDecl(VariableDefinitionNode* varDecl);
    void generateLetDecl(VariableDefinitionNode* letDecl);
    void generatePrintStmt(ASTNode* printStmt);
    void generateFunctionCall(MethodCallNode* funcCall);

    void generateNewExpr(NewExprNode* newExpr, x86::Gp destReg, x86::Gp sourceScopeReg = x86::r15);
    void generateMemberAccess(MemberAccessNode* memberAccess, x86::Gp destReg);
    void generateMemberAssign(MemberAssignNode* memberAssign);
    void generateRawMemoryRelease(MethodCallNode* methodCall);
    bool isRawMemoryReleaseCall(MethodCallNode* methodCall) const;
    void generateClassDecl(ClassDeclarationNode* classDecl);
    
    // Assembly library wrapper methods for internal use
    void makeSafeUnorderedList(x86::Gp addressReg, x86::Gp offsetReg, int32_t initialSize = 16);
    void addToSafeList(x86::Gp addressReg, x86::Gp offsetReg, x86::Gp valueReg);
    void removeFromSafeList(x86::Gp addressReg, x86::Gp offsetReg, x86::Gp indexReg);
    void compactSafeList(x86::Gp addressReg, x86::Gp offsetReg);
    
    // Patch method addresses into metadata closures after code commit
    void patchMetadataClosures(void* codeBase, const std::map<std::string, ClassDeclarationNode*>& classRegistry);
    
    // Function-related utilities
    void createFunctionLabel(FunctionDeclarationNode* funcDecl);
    void generateFunctionPrologue(FunctionDeclarationNode* funcDecl);
    void generateFunctionEpilogue(FunctionDeclarationNode* funcDecl);
    void storeFunctionAddressInClosure(FunctionDeclarationNode* funcDecl, LexicalScopeNode* scope);
    
    // Generic scope management utilities (shared by functions and blocks)
    void generateScopePrologue(LexicalScopeNode* scope);
    void generateScopeEpilogue(LexicalScopeNode* scope);
    
    // Metadata generation for GC
    // Scope metadata is created ONCE at compile time and stored in scope->metadata
    void initializeAllScopeMetadata(ASTNode* root, const std::vector<FunctionDeclarationNode*>& functionRegistry);
    void initializeScopeMetadataRecursive(ASTNode* node);  // Helper for recursive traversal
    ScopeMetadata* createScopeMetadata(LexicalScopeNode* scope);  // Returns ScopeMetadata* now that it's forward declared
    
    // Block statement utilities
    void generateBlockStmt(BlockStatement* blockStmt);
    
    // Code generation utilities
    void setupMainFunction();
    void cleanupMainFunction();
    void loadValue(ASTNode* valueNode, x86::Gp destReg, x86::Gp sourceScopeReg = x86::r15, std::optional<DataType> expectedType = std::nullopt);
    void loadAnyValue(ASTNode* valueNode, x86::Gp valueReg, x86::Gp typeReg, x86::Gp sourceScopeReg = x86::r15);
    void storeVariableInScope(const std::string& varName, x86::Gp valueReg, LexicalScopeNode* scope, ASTNode* valueNode = nullptr, x86::Gp typeReg = x86::rdx);
    void loadVariableFromScope(IdentifierExpressionNode* identifier, x86::Gp destReg, int offsetInVariable = 0, x86::Gp sourceScopeReg = x86::r15);
    void loadVariableAddress(IdentifierExpressionNode* identifier, x86::Gp destReg, int offsetInVariable = 0, x86::Gp sourceScopeReg = x86::r15);
    void loadParameterIntoRegister(int paramIndex, x86::Gp destReg, x86::Gp scopeReg = x86::r15);
    x86::Gp getParameterByIndex(int paramIndex);
    
    // Tensor operation utilities
    int vtableOffsetForOperatorIndex; // Offset in vtable for operator[] function
    
    // External function declarations
    void declareExternalFunctions();
    Label printInt64Label;
    Label mallocLabel;
    Label freeLabel;
    Label callocLabel;
};

// Main interface class expected by main.cpp
class Codegen {
public:
    Codegen();
    ~Codegen();
    
    void generateProgram(ASTNode& root, const std::map<std::string, ClassDeclarationNode*>& classRegistry);
    void run();
    
private:
    CodeGenerator generator;
    void* generatedFunction;
};

// External C functions that will be linked
extern "C" {
    void* malloc_wrapper(size_t size);
    void* calloc_wrapper(size_t nmemb, size_t size);
    void free_wrapper(void* ptr);
}
