#include "codegen.h"
#include "gc.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
// #include "codegen_torch.h"
#include "codegen_array.h"

// External C functions implementation
extern "C" {
    void* malloc_wrapper(size_t size) {
        return malloc(size);
    }
    
    void* calloc_wrapper(size_t nmemb, size_t size) {
        return calloc(nmemb, size);
    }
    
    void free_wrapper(void* ptr) {
        free(ptr);
    }
}

// Codegen class implementation (main interface)
Codegen::Codegen() : generatedFunction(nullptr) {
}

Codegen::~Codegen() {
    // Function cleanup is handled by asmjit runtime
}

void Codegen::generateProgram(ASTNode& root, const std::map<std::string, ClassDeclarationNode*>& classRegistry) {
    generatedFunction = generator.generateCode(&root, classRegistry);
}

void Codegen::run() {
    if (!generatedFunction) {
        throw std::runtime_error("No generated function to run");
    }
    
    std::cout << "\n=== Executing Generated Code ===" << std::endl;
    
    // Cast to function pointer and call
    typedef int (*MainFunc)();
    MainFunc func = reinterpret_cast<MainFunc>(generatedFunction);
    int result = func();
    
    std::cout << "=== Execution Complete (returned " << result << ") ===" << std::endl;
}

// CodeGenerator class implementation
CodeGenerator::CodeGenerator() : currentScope(nullptr) {
    // Builder will be initialized in generateCode when code holder is ready
    
    // Initialize Capstone for disassembly
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &capstoneHandle) != CS_ERR_OK) {
        throw std::runtime_error("Failed to initialize Capstone disassembler");
    }
    cs_option(capstoneHandle, CS_OPT_DETAIL, CS_OPT_ON);
}

CodeGenerator::~CodeGenerator() {
    cs_close(&capstoneHandle);
}

void* CodeGenerator::generateCode(ASTNode* root, const std::map<std::string, ClassDeclarationNode*>& classRegistry) {
    // Reset and initialize code holder
    code.reset();
    code.init(rt.environment(), rt.cpuFeatures());

    // Create builder with the code holder
    cb = new x86::Builder(&code);

    // Initialize AsmLibrary with the current builder
    asmLibrary = std::make_unique<AsmLibrary>(*cb, x86::r15);

    std::cout << "=== Generated Assembly Code ===" << std::endl;

    // PRE-PROCESSING: Handle block statement roots by wrapping them in a main function
    if (root->nodeType == ASTNodeType::BLOCK_STATEMENT) {
        std::cout << "Pre-processing: Wrapping block statement root in main function" << std::endl;

        // Create a main function that wraps the block statement
        FunctionDeclarationNode* mainFunc = new FunctionDeclarationNode(nullptr);
        mainFunc->name = "main";
        mainFunc->funcName = "main";
        mainFunc->body = static_cast<BlockStatement*>(root);

        // Copy variables from the root block statement to the main function
        BlockStatement* rootBlock = static_cast<BlockStatement*>(root);
        mainFunc->variables = rootBlock->variables;

        // Calculate totalSize for the main function based on its variables
        if (!mainFunc->variables.empty()) {
            int maxOffset = 0;
            for (const auto& [name, varInfo] : mainFunc->variables) {
                maxOffset = std::max(maxOffset, varInfo.offset + varInfo.size);
            }
            mainFunc->totalSize = maxOffset;
        } else {
            mainFunc->totalSize = 16; // Minimum for metadata and flags
        }

        std::cout << "DEBUG: Main function totalSize set to " << mainFunc->totalSize << std::endl;
        std::cout << "DEBUG: Main function has " << mainFunc->variables.size() << " variables" << std::endl;

        // Create metadata for the main function
        mainFunc->metadata = createScopeMetadata(mainFunc);

        // Set the parent of the block to the main function
        root->parent = mainFunc;

        // Replace root with the main function
        root = mainFunc;

        std::cout << "Wrapped block statement in main function" << std::endl;
    }

    // SECOND PASS: Generate the main program flow (this traverses the AST normally)
    // Classes will be emitted as they appear in the AST, creating closures for methods inline
    std::cout << "\n=== Generating Main Program ===" << std::endl;
    generateProgram(root);
    
    std::cout << "Code size after program: " << code.codeSize() << std::endl;
    
    // Finalize the code (this resolves all forward references)
    cb->finalize();
    
    std::cout << "Final code size: " << code.codeSize() << std::endl;
    
    // Clean up builder
    delete cb;
    cb = nullptr;
    
    // Commit the code to executable memory
    void* executableFunc;
    Error err = rt.add(&executableFunc, &code);
    if (err) {
        std::cout << "Error details: " << DebugUtils::errorAsString(err) << std::endl;
        std::cout << "Code size: " << code.codeSize() << std::endl;
        throw std::runtime_error("Failed to generate code: " + std::string(DebugUtils::errorAsString(err)));
    }
    
    std::cout << "Successfully generated code, size: " << code.codeSize() << " bytes" << std::endl;
    
    // NOW patch the metadata closures with actual function addresses
    patchMetadataClosures(executableFunc, classRegistry);
    
    // Get the code size for disassembly
    size_t codeSize = code.codeSize();
    
    // Disassemble and print the generated code
    disassembleAndPrint(executableFunc, codeSize);
    
    return executableFunc;
}





void CodeGenerator::generateProgram(ASTNode* root) {
    if (!root) {
        throw std::runtime_error("Null program root");
    }

    std::cout << "Generating program" << std::endl;

    // Handle different root node types
    if (root->nodeType == ASTNodeType::FUNCTION_DECLARATION) {
        // Root is a function - generate it
        FunctionDeclarationNode* mainFunc = static_cast<FunctionDeclarationNode*>(root);

        // Create label and bind it
        createFunctionLabel(mainFunc);
        cb->bind(functionLabels[mainFunc]);

        // Generate function prologue
        generateFunctionPrologue(mainFunc);

        // Set current scope
        currentScope = mainFunc;

        // Generate code for the function body
        for (auto* child : mainFunc->children) {
            if (child->nodeType != ASTNodeType::FUNCTION_DECLARATION && child->nodeType != ASTNodeType::CLASS_DECLARATION) {
                visitNode(child);
            }
        }

        // For main function, set return value
        cb->mov(x86::eax, 0);

        // Generate function epilogue
        generateFunctionEpilogue(mainFunc);
    } else {
        throw std::runtime_error("Invalid program root node type - expected FUNCTION_DECLARATION");
    }
}

void CodeGenerator::visitNode(ASTNode* node) {
    if (!node) return;

    // Handle print statements
    if (node->value == "print") {
        generatePrintStmt(node);
        return;
    }

    switch (node->nodeType) {
        case ASTNodeType::VARIABLE_DEFINITION:
            generateVarDecl(static_cast<VariableDefinitionNode*>(node));
            break;
        case ASTNodeType::FUNCTION_DECLARATION:
            // Function bodies are generated during the upfront function pass.
            break;
        case ASTNodeType::BLOCK_STATEMENT:
            generateBlockStmt(static_cast<BlockStatement*>(node));
            break;
        case ASTNodeType::CLASS_DECLARATION:
            generateClassDecl(static_cast<ClassDeclarationNode*>(node));
            break;
        default:
            // For other nodes, just visit children
            for (auto* child : node->children) {
                visitNode(child);
            }
            break;
    }
}

void CodeGenerator::allocateScope(LexicalScopeNode* scope) {
    std::cout << "Allocating scope of size: " << scope->totalSize << " bytes" << std::endl;

    // Save parent scope register (r14) - r15 doesn't need to be saved since its value goes into r14
    cb->push(x86::r14);
    // Set r14 to current r15 (the new scope's parent will be the current scope)
    cb->mov(x86::r14, x86::r15);
    
    
    // Call calloc to allocate and zero-initialize scope
    // mov rdi, 1 (number of elements)
    cb->mov(x86::rdi, 1);
    // mov rsi, scope->totalSize (size of each element)
    cb->mov(x86::rsi, scope->totalSize);
    
    // Call calloc - we need to call the actual calloc function
    // For simplicity, we'll embed the function pointer directly
    uint64_t callocAddr = reinterpret_cast<uint64_t>(&calloc_wrapper);
    cb->mov(x86::rax, callocAddr);
    cb->call(x86::rax);
    
    
    // Store the allocated memory address in r15
    cb->mov(x86::r15, x86::rax);
    
    // Store pre-computed metadata pointer at offset 8 in the scope
    // The metadata was created at compile time, so we just load the pointer
    if (!scope->metadata) {
        // For synthetic main function, metadata might not be set yet - create it now
        if (auto funcScope = dynamic_cast<FunctionDeclarationNode*>(scope)) {
            if (funcScope->name == "main") {
                scope->metadata = createScopeMetadata(scope);
                std::cout << "Created metadata for synthetic main function at runtime" << std::endl;
            } else {
                throw std::runtime_error("Scope metadata not initialized at compile time!");
            }
        } else {
            throw std::runtime_error("Scope metadata not initialized at compile time!");
        }
    }
    cb->mov(x86::r11, reinterpret_cast<uint64_t>(scope->metadata));
    cb->mov(x86::qword_ptr(x86::r15, ScopeLayout::METADATA_OFFSET), x86::r11);
    
    // Track scope as an allocated object for GC
    cb->mov(x86::rdi, x86::r15);  // First argument: scope pointer
    uint64_t gcTrackAddr = reinterpret_cast<uint64_t>(&gc_track_object);
    cb->mov(x86::r11, gcTrackAddr);
    cb->call(x86::r11);
    
    // Track scope in GC (push scope to roots)
    cb->mov(x86::rdi, x86::r15);  // First argument: scope pointer
    uint64_t gcPushScopeAddr = reinterpret_cast<uint64_t>(&gc_push_scope);
    cb->mov(x86::r11, gcPushScopeAddr);
    cb->call(x86::r11);
    
    currentScope = scope;
}

ScopeMetadata* CodeGenerator::createScopeMetadata(LexicalScopeNode* scope) {
    if (!scope) return nullptr;

    std::cout << "DEBUG createScopeMetadata: Creating metadata for scope with " << scope->variables.size() << " variables" << std::endl;

    // This function is now called at COMPILE TIME (during code generation setup)
    // NOT at runtime! The metadata is created once and reused for all scope allocations.

    // Count variables that need GC tracking (objects and closures)
    std::vector<VarMetadata> trackedVars;

    for (const auto& [varName, varInfo] : scope->variables) {
        // Only track object references and closures (anything that could point to heap objects)
        if (varInfo.type == DataType::OBJECT || varInfo.type == DataType::CLOSURE) {
            void* typeInfo = nullptr;

            // For objects, get the class metadata from the registry
            if (varInfo.type == DataType::OBJECT && varInfo.classNode) {
                typeInfo = MetadataRegistry::getInstance().getClassMetadata(varInfo.classNode->className);
            }

            std::cout << "  - Tracking variable '" << varName << "' of type "
                      << (varInfo.type == DataType::OBJECT ? "OBJECT" : "CLOSURE")
                      << " at offset " << varInfo.offset << std::endl;

            // Note: offset in VariableInfo is already adjusted for ScopeLayout::DATA_OFFSET
            // But we need the offset relative to data start for the metadata
            trackedVars.emplace_back(varInfo.offset, varInfo.type, typeInfo);
        }
    }

    // Allocate metadata structure ONCE at compile time
    ScopeMetadata* metadata = new ScopeMetadata();
    metadata->numVars = trackedVars.size();

    if (metadata->numVars > 0) {
        metadata->vars = new VarMetadata[metadata->numVars];
        for (int i = 0; i < metadata->numVars; i++) {
            metadata->vars[i] = trackedVars[i];
        }
    } else {
        metadata->vars = nullptr;
    }

    std::cout << "Created scope metadata at compile time with " << metadata->numVars << " tracked variables" << std::endl;

    return metadata;
}



void CodeGenerator::initializeScopeMetadataRecursive(ASTNode* node) {
    if (!node) return;
    
    // If this is a lexical scope (block), create its metadata
    if (node->nodeType == ASTNodeType::BLOCK_STATEMENT) {
        BlockStatement* block = static_cast<BlockStatement*>(node);
        if (!block->metadata) {
            block->metadata = createScopeMetadata(block);
            std::cout << "    Created metadata for block at depth " << block->depth << std::endl;
        }
    }

    // Recursively process children (skip nested functions - they're handled in the main loop)
    for (auto& child : node->children) {
        if (child->nodeType != ASTNodeType::FUNCTION_DECLARATION && child->nodeType != ASTNodeType::CLASS_DECLARATION) {
            initializeScopeMetadataRecursive(child);
        }
    }
}

void CodeGenerator::generateVarDecl(VariableDefinitionNode* varDecl) {
    // For now, we expect a literal assignment
    // var x: int64 = 10
    if (!varDecl->initializer) {
        // Variable declaration without assignment, set to 0
        cb->mov(x86::rax, 0);
        storeVariableInScope(varDecl->name, x86::rax, currentScope, nullptr);
        return;
    }

    if (varDecl->isArray) {
        // For now, assume arrays are not supported - we'll need to update this
        throw std::runtime_error("Array variables not implemented yet");
    } else {
        ASTNode* valueNode = varDecl->initializer;
        // For now, assume all variables are INT64 - we'll need to update this for proper type handling
        loadValue(valueNode, x86::rax, x86::r15, DataType::INT64);
        storeVariableInScope(varDecl->name, x86::rax, currentScope, valueNode);
    }
}

void CodeGenerator::generateLetDecl(VariableDefinitionNode* letDecl) {
    std::cout << "Generating let declaration: " << letDecl->name << std::endl;

    // Let declarations work the same as var declarations
    // let x: int64 = 10
    if (letDecl->initializer == nullptr) {
        throw std::runtime_error("Let declaration without assignment not supported");
    }

    ASTNode* valueNode = letDecl->initializer;

    // Load the value into a register using declared type
    loadValue(valueNode, x86::rax, x86::r15, DataType::INT64); // TODO: get type from typeAnnotation

    // Store the value in the current scope
    storeVariableInScope(letDecl->name, x86::rax, currentScope, valueNode);
}



void CodeGenerator::loadValue(ASTNode* valueNode, x86::Gp destReg, x86::Gp sourceScopeReg, std::optional<DataType> expectedType) {
    if (!valueNode) return;

    // std::cout << "DEBUG loadValue: node type = " << static_cast<int>(valueNode->nodeType) << std::endl;

    switch (valueNode->nodeType) {
        case ASTNodeType::LITERAL_EXPRESSION: {
            auto* literal = static_cast<LiteralExpressionNode*>(valueNode);
            DataType parseType = expectedType.value_or(DataType::INT64);
            switch (parseType) {
                case DataType::INT64: {
                    int64_t value = std::stoll(literal->literal);
                    cb->mov(destReg, value);
                    break;
                }
                case DataType::FLOAT64: {
                    double value = std::stod(literal->literal);
                    uint64_t bits;
                    std::memcpy(&bits, &value, sizeof(bits));
                    cb->mov(destReg, bits);
                    break;
                }

                case DataType::STRING: {
                    uint64_t strAddr = reinterpret_cast<uint64_t>(literal->literal.c_str());
                    cb->mov(destReg, strAddr);
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported expected type for literal");
            }
            break;
        }
        case ASTNodeType::IDENTIFIER_EXPRESSION: {
            // Load variable from scope (using provided source scope register)
            loadVariableFromScope(static_cast<IdentifierExpressionNode*>(valueNode), destReg, 0, sourceScopeReg);
            break;
        }
        case ASTNodeType::AWAIT_EXPRESSION: {
            // Handle await expression - not implemented yet
            throw std::runtime_error("Await expressions not implemented");
            break;
        }
        case ASTNodeType::NEW_EXPR: {
            // Handle new expression
            generateNewExpr(static_cast<NewExprNode*>(valueNode), destReg, sourceScopeReg);
            break;
        }
        case ASTNodeType::MEMBER_ACCESS: {
            // Handle member access
            generateMemberAccess(static_cast<MemberAccessNode*>(valueNode), destReg);
            break;
        }
        case ASTNodeType::EXPRESSION: {
            std::cout << "  -> Matched EXPRESSION case" << std::endl;
            // Expression is a wrapper, recurse to the actual expression
            if (!valueNode->children.empty()) {
                loadValue(valueNode->children[0], destReg, sourceScopeReg, expectedType);
            } else {
                throw std::runtime_error("Empty expression node in loadValue");
            }
            break;
        }
        case ASTNodeType::THIS_EXPR: {
            // Handle 'this' expression - load from first parameter
            // 'this' is always the first parameter in a method
            // It's stored in the scope like any other variable

            // Create a temporary identifier node to load 'this' variable
            IdentifierExpressionNode tempIdentifier(nullptr, "this");
            tempIdentifier.varRef = nullptr;
            tempIdentifier.accessedIn = currentScope;

            // Find 'this' in the current scope
            auto it = currentScope->variables.find("this");
            if (it == currentScope->variables.end()) {
                throw std::runtime_error("'this' not found in method scope");
            }
            tempIdentifier.varRef = &it->second;

            // Load 'this' from scope (using provided source scope register)
            loadVariableFromScope(&tempIdentifier, destReg, 0, sourceScopeReg);
            break;
        }
        case ASTNodeType::TYPE_ANNOTATION: {
            // Type annotations are not values and should not be passed to loadValue
            throw std::runtime_error("Type annotation node passed to loadValue - this is a parser/codegen bug");
            break;
        }
        default:
            std::cout << "DEBUG loadValue: Unsupported node type " << static_cast<int>(valueNode->nodeType) << " (default case)" << std::endl;
            throw std::runtime_error("Unsupported value node type in loadValue");
    }
}

void CodeGenerator::loadAnyValue(ASTNode* valueNode, x86::Gp valueReg, x86::Gp typeReg, x86::Gp sourceScopeReg) {
    if (!valueNode) {
        throw std::runtime_error("Null value node for any load");
    }

    switch (valueNode->nodeType) {
        case ASTNodeType::LITERAL_EXPRESSION: {
            auto* literal = static_cast<LiteralExpressionNode*>(valueNode);
            // Assume it's a number for now
            double floatValue = std::stod(literal->literal);
            uint64_t bits;
            std::memcpy(&bits, &floatValue, sizeof(bits));
            cb->mov(valueReg, bits);
            cb->mov(typeReg, static_cast<uint32_t>(DataType::FLOAT64));
            break;
        }
        case ASTNodeType::IDENTIFIER_EXPRESSION: {
            auto* identifier = static_cast<IdentifierExpressionNode*>(valueNode);
            if (!identifier->varRef) {
                throw std::runtime_error("Identifier not analyzed for any load: " + identifier->name);
            }

            switch (identifier->varRef->type) {
                case DataType::INT64:
                case DataType::FLOAT64:
                case DataType::OBJECT:
                case DataType::RAW_MEMORY:
                case DataType::STRING:
                    loadVariableFromScope(identifier, valueReg, 0, sourceScopeReg);
                    cb->mov(typeReg, static_cast<uint32_t>(identifier->varRef->type));
                    break;
                default:
                    throw std::runtime_error("Unsupported identifier type for any value: " + identifier->name);
            }
            break;
        }
        case ASTNodeType::NEW_EXPR: {
            auto* newExpr = static_cast<NewExprNode*>(valueNode);
            generateNewExpr(newExpr, valueReg, sourceScopeReg);
            DataType resultType = newExpr->isRawMemory ? DataType::RAW_MEMORY : DataType::OBJECT;
            cb->mov(typeReg, static_cast<uint32_t>(resultType));
            break;
        }
        case ASTNodeType::MEMBER_ACCESS: {
            auto* memberAccess = static_cast<MemberAccessNode*>(valueNode);
            if (!memberAccess->classRef) {
                throw std::runtime_error("Class reference missing for member access in any value");
            }
            auto fieldIt = memberAccess->classRef->fields.find(memberAccess->memberName);
            if (fieldIt == memberAccess->classRef->fields.end()) {
                throw std::runtime_error("Field not found for member access in any value: " + memberAccess->memberName);
            }
            DataType fieldType = fieldIt->second.type;

            x86::Gp objectReg = x86::r11;
            loadValue(memberAccess->object, objectReg);

            if (fieldType == DataType::ANY) {
                cb->mov(typeReg, x86::qword_ptr(objectReg, memberAccess->memberOffset));
                cb->mov(valueReg, x86::qword_ptr(objectReg, memberAccess->memberOffset + 8));
            } else {
                generateMemberAccess(memberAccess, valueReg);
                cb->mov(typeReg, static_cast<uint32_t>(fieldType));
            }
            break;
        }
        default:
            throw std::runtime_error("Unsupported node type for any value");
    }
}

void CodeGenerator::storeVariableInScope(const std::string& varName, x86::Gp valueReg, LexicalScopeNode* scope, ASTNode* valueNode, x86::Gp typeReg) {
    // Find the variable in the scope
    auto it = scope->variables.find(varName);
    if (it == scope->variables.end()) {
        throw std::runtime_error("Variable not found in scope: " + varName);
    }
    
    int offset = it->second.offset;
    std::cout << "Storing variable '" << varName << "' at offset " << offset << " in scope" << std::endl;
    
    // Store the value at [r15 + offset]
    cb->mov(x86::ptr(x86::r15, offset), valueReg);
    
    // Store the value at [r15 + offset]
    cb->mov(x86::ptr(x86::r15, offset), valueReg);
    
    // If this is an object-typed variable and not a NEW expression, handle GC write barrier inline
    if (it->second.type == DataType::OBJECT && valueNode && valueNode->nodeType != ASTNodeType::NEW_EXPR) {
        // Inline GC write barrier - check needs_set_flag and atomically set set_flag if needed
        // This is much faster than a function call
        
        // Load flags from object header (at offset 8)
        // flags is at [valueReg + 8]
        cb->mov(x86::rcx, x86::qword_ptr(valueReg, ObjectLayout::FLAGS_OFFSET));  // Load flags
        
        // Test if NEEDS_SET_FLAG (bit 0) is set
        cb->test(x86::rcx, ObjectFlags::NEEDS_SET_FLAG);
        
        // Create a label for skipping if not needed
        Label skipWriteBarrier = cb->newLabel();
        cb->jz(skipWriteBarrier);  // Jump if zero (NEEDS_SET_FLAG not set)
        
        // NEEDS_SET_FLAG is set, so OR the SET_FLAG bit (no LOCK needed - idempotent)
        // Using non-locked OR is safe because we're only setting a bit to 1
        cb->or_(x86::qword_ptr(valueReg, ObjectLayout::FLAGS_OFFSET), ObjectFlags::SET_FLAG);
        
        // Memory fence to ensure write visibility to GC thread
        // This guarantees the set_flag write is visible before GC reads it in phase 3
        cb->mfence();
        
        cb->bind(skipWriteBarrier);
    }
}

void CodeGenerator::loadParameterIntoRegister(int paramIndex, x86::Gp destReg, x86::Gp scopeReg) {
    // Load a parameter from the specified scope. Parameters can be:
    // - For functions: regular parameters + hidden parameters (parent scope pointers)
    // - For blocks: only hidden parameters (parent scope pointers)
    
    if (auto currentFunc = dynamic_cast<FunctionDeclarationNode*>(currentScope)) {
        // Current scope is a function
        size_t totalRegularParams = currentFunc->paramsInfo.size();
        
        if (static_cast<size_t>(paramIndex) < totalRegularParams) {
            // This is a regular parameter - load from its scope offset
            const VariableInfo& param = currentFunc->paramsInfo[paramIndex];
            std::cout << "Loading regular parameter " << paramIndex << " from scope offset " << param.offset << " using scope register" << std::endl;
            cb->mov(destReg, x86::ptr(scopeReg, param.offset));
        } else {
            // This is a hidden parameter - load from its scope offset
            size_t hiddenParamIndex = paramIndex - totalRegularParams;
            if (hiddenParamIndex >= currentFunc->hiddenParamsInfo.size()) {
                throw std::runtime_error("Hidden parameter index out of range");
            }
            
            const ParameterInfo& hiddenParam = currentFunc->hiddenParamsInfo[hiddenParamIndex];
            std::cout << "Loading hidden parameter " << hiddenParamIndex << " (total param index " << paramIndex << ") from scope offset " << hiddenParam.offset << " using scope register" << std::endl;
            cb->mov(destReg, x86::ptr(scopeReg, hiddenParam.offset));
        }
    } else {
        // Current scope is a block - only has hidden parameters (parent scope pointers)
        // For blocks, paramIndex directly maps to the index of the parent scope pointer
        std::cout << "Loading parent scope pointer " << paramIndex << " from block scope offset " << (paramIndex * 8) << " using scope register" << std::endl;
        cb->mov(destReg, x86::ptr(scopeReg, paramIndex * 8));
    }
}

x86::Gp CodeGenerator::getParameterByIndex(int paramIndex) {
    // System V ABI parameter registers: rdi, rsi, rdx, rcx, r8, r9
    x86::Gp paramRegs[] = {x86::rdi, x86::rsi, x86::rdx, x86::rcx, x86::r8, x86::r9};
    const int maxRegParams = 6;
    
    if (paramIndex < maxRegParams) {
        // Parameter is in a register
        std::cout << "Parameter " << paramIndex << " is in register" << std::endl;
        return paramRegs[paramIndex];
    } else {
        // Parameter is on stack - load it into a temporary register (rax)
        std::cout << "Parameter " << paramIndex << " is on stack, loading to rax" << std::endl;
        
        // Calculate stack offset for this parameter
        // Stack layout: [return_addr][saved_rbp][saved_r15][stack_params...]
        int stackOffset = 24 + (paramIndex - maxRegParams) * 8;
        cb->mov(x86::rax, x86::ptr(x86::rbp, stackOffset));
        
        return x86::rax;
    }
}

void CodeGenerator::loadVariableFromScope(IdentifierExpressionNode* identifier, x86::Gp destReg, int offsetInVariable, x86::Gp sourceScopeReg) {
    if (!identifier->varRef) {
        throw std::runtime_error("Variable reference not analyzed: " + identifier->value);
    }
    
    // Get the variable access information
    auto access = identifier->getVariableAccess();
    
    if (access.inCurrentScope) {
        // Variable is in current scope (use provided source scope register)
        std::cout << "Loading variable '" << identifier->value << "' from current scope at offset " << access.offset << " with additional offset " << offsetInVariable << std::endl;
        cb->mov(destReg, x86::ptr(sourceScopeReg, access.offset + offsetInVariable));
    } else {
        // Variable is in a parent scope - load the parent scope pointer from our lexical scope first
        std::cout << "Loading variable '" << identifier->value << "' from parent scope parameter index " << access.scopeParameterIndex << " at offset " << access.offset << " with additional offset " << offsetInVariable << std::endl;
        
        // Load parent scope pointer from our lexical scope into a temporary register
        loadParameterIntoRegister(access.scopeParameterIndex, x86::rax, sourceScopeReg);
        // Now load the variable from that parent scope
        cb->mov(destReg, x86::ptr(x86::rax, access.offset + offsetInVariable));
    }
}

void CodeGenerator::loadVariableAddress(IdentifierExpressionNode* identifier, x86::Gp destReg, int offsetInVariable, x86::Gp sourceScopeReg) {
    if (!identifier->varRef) {
        throw std::runtime_error("Variable reference not analyzed: " + identifier->value);
    }
    
    // Get the variable access information
    auto access = identifier->getVariableAccess();
    
    if (access.inCurrentScope) {
        // Variable is in current scope - load address using LEA (use provided source scope register)
        std::cout << "Loading address of variable '" << identifier->value << "' from current scope at offset " << access.offset << " with additional offset " << offsetInVariable << std::endl;
        cb->lea(destReg, x86::ptr(sourceScopeReg, access.offset + offsetInVariable));
    } else {
        // Variable is in a parent scope - load the parent scope pointer from our lexical scope first
        std::cout << "Loading address of variable '" << identifier->value << "' from parent scope parameter index " << access.scopeParameterIndex << " at offset " << access.offset << " with additional offset " << offsetInVariable << std::endl;
        
        // Load parent scope pointer from our lexical scope into a temporary register
        loadParameterIntoRegister(access.scopeParameterIndex, x86::rax, sourceScopeReg);
        // Now load the address from that parent scope
        cb->lea(destReg, x86::ptr(x86::rax, access.offset + offsetInVariable));
    }
}

void CodeGenerator::generatePrintStmt(ASTNode* printStmt) {
    if (printStmt->children.empty()) {
        throw std::runtime_error("Print statement without argument");
    }

    ASTNode* arg = printStmt->children[0];
    // Unwrap parenthesis expression
    if (arg->nodeType == ASTNodeType::PARENTHESIS_EXPRESSION && !arg->children.empty()) {
        arg = arg->children[0];
    }
    
    DataType detectedType = DataType::INT64;
    if (arg->nodeType == ASTNodeType::LITERAL_EXPRESSION) {
        auto* literal = static_cast<LiteralExpressionNode*>(arg);
        detectedType = (literal->literal.find('"') != std::string::npos) ? DataType::STRING : DataType::INT64;
    } else if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
        auto* identifier = static_cast<IdentifierExpressionNode*>(arg);
        if (identifier->varRef) {
            detectedType = identifier->varRef->type;
        }
    } else if (arg->nodeType == ASTNodeType::MEMBER_ACCESS) {
        auto* memberAccess = static_cast<MemberAccessNode*>(arg);
        if (memberAccess->classRef) {
            auto it = memberAccess->classRef->fields.find(memberAccess->memberName);
            if (it != memberAccess->classRef->fields.end()) {
                detectedType = it->second.type;
            }
        }
    }
    
    switch (detectedType) {
        case DataType::ANY: {
            if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                auto* identifier = static_cast<IdentifierExpressionNode*>(arg);
                loadVariableFromScope(identifier, x86::rdi, 0);
                loadVariableFromScope(identifier, x86::rsi, 8);
            } else {
                loadAnyValue(arg, x86::rsi, x86::rdi);
            }
            uint64_t printAddr = reinterpret_cast<uint64_t>(&print_any);
            cb->sub(x86::rsp, 8); // Maintain 16-byte alignment for variadic call
            cb->mov(x86::rax, printAddr);
            cb->call(x86::rax);
            cb->add(x86::rsp, 8);
            break;
        }
        case DataType::FLOAT64: {
            uint64_t bits = 0;
            if (arg->nodeType == ASTNodeType::LITERAL_EXPRESSION) {
                double floatValue = std::stod(static_cast<LiteralExpressionNode*>(arg)->literal);
                std::memcpy(&bits, &floatValue, sizeof(bits));
                cb->mov(x86::rax, bits);
            } else if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                loadVariableFromScope(static_cast<IdentifierExpressionNode*>(arg), x86::rax);
            } else if (arg->nodeType == ASTNodeType::MEMBER_ACCESS) {
                generateMemberAccess(static_cast<MemberAccessNode*>(arg), x86::rax);
            } else {
                throw std::runtime_error("Unsupported expression for float64 print");
            }
            cb->movq(x86::xmm0, x86::rax);
            uint64_t printAddr = reinterpret_cast<uint64_t>(&print_float64);
            cb->mov(x86::rax, printAddr);
            cb->call(x86::rax);
            break;
        }
        case DataType::STRING: {
            if (arg->nodeType != ASTNodeType::LITERAL_EXPRESSION) {
                throw std::runtime_error("String print currently supports only literals");
            }
            auto* literal = static_cast<LiteralExpressionNode*>(arg);
            uint64_t strAddr = reinterpret_cast<uint64_t>(literal->literal.c_str());
            cb->mov(x86::rdi, strAddr);
            uint64_t printAddr = reinterpret_cast<uint64_t>(&print_string);
            cb->mov(x86::rax, printAddr);
            cb->call(x86::rax);
            break;
        }
        default: {
            if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                loadVariableFromScope(static_cast<IdentifierExpressionNode*>(arg), x86::rdi);
            } else {
                loadValue(arg, x86::rdi);
            }
            uint64_t printAddr = reinterpret_cast<uint64_t>(&print_int64);
            cb->mov(x86::rax, printAddr);
            cb->call(x86::rax);
            break;
        }
    }
}

void CodeGenerator::printInt64(IdentifierExpressionNode* identifier) {
    // Load the variable value into rdi (first argument for calling convention)
    loadVariableFromScope(identifier, x86::rdi);

    std::cout << "Generating call to print_int64" << std::endl;

    // Call the external print function
    uint64_t printAddr = reinterpret_cast<uint64_t>(&print_int64);
    cb->mov(x86::rax, printAddr);
    cb->call(x86::rax);
}

void CodeGenerator::patchMetadataClosures(void* codeBase, const std::map<std::string, ClassDeclarationNode*>& classRegistry) {
    std::cout << "\n=== Patching Metadata Closures ===" << std::endl;
    
    // Iterate through all classes and patch their method closures
    for (const auto& [className, classDecl] : classRegistry) {
        ClassMetadata* metadata = MetadataRegistry::getInstance().getClassMetadata(className);
        if (!metadata) {
            std::cout << "WARNING: No metadata found for class " << className << std::endl;
            continue;
        }
        
        std::cout << "Patching " << metadata->numMethods << " methods for class " << className << std::endl;
        
        // Patch each method closure
        for (size_t i = 0; i < classDecl->methodLayout.size(); i++) {
            const auto& methodInfo = classDecl->methodLayout[i];
            FunctionDeclarationNode* method = methodInfo.method;
            
            if (!method || !method->asmjitLabel) {
                std::cout << "WARNING: No label for method " << methodInfo.methodName << std::endl;
                continue;
            }
            
            // Get the label and resolve its offset
            Label* label = static_cast<Label*>(method->asmjitLabel);
            uint32_t labelId = label->id();
            
            if (labelId == Globals::kInvalidId) {
                throw std::runtime_error("Invalid label ID for method: " + methodInfo.methodName);
            }
            
            LabelEntry* labelEntry = code.labelEntry(labelId);
            if (!labelEntry) {
                throw std::runtime_error("Label entry not found for method: " + methodInfo.methodName);
            }
            
            size_t labelOffset = labelEntry->offset();
            void* funcAddr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(codeBase) + labelOffset);
            
            // Patch the closure in metadata
            Closure* closure = metadata->methodClosures[i];
            closure->funcAddr = funcAddr;
            
            std::cout << "  Patched " << className << "::" << methodInfo.methodName 
                      << " -> " << funcAddr << " (offset: 0x" << std::hex << labelOffset << std::dec << ")" << std::endl;
        }
    }
    
    std::cout << "=== Patching Complete ===" << std::endl;
}

void CodeGenerator::disassembleAndPrint(void* code, size_t codeSize) {
    cs_insn* insn;
    size_t count = cs_disasm(capstoneHandle, 
                            static_cast<const uint8_t*>(code), 
                            codeSize, 
                            reinterpret_cast<uint64_t>(code), 
                            0, 
                            &insn);
    
    if (count > 0) {
        std::cout << "\n=== Disassembled Code ===" << std::endl;
        for (size_t i = 0; i < count; i++) {
            printf("0x%016lx:  %-12s %s\n", 
                   insn[i].address, 
                   insn[i].mnemonic, 
                   insn[i].op_str);
        }
        std::cout << "=========================\n" << std::endl;
        cs_free(insn, count);
    } else {
        std::cout << "Failed to disassemble code" << std::endl;
    }
}

void CodeGenerator::createFunctionLabel(FunctionDeclarationNode* funcDecl) {
    // Create a new label for this function
    Label funcLabel = cb->newLabel();
    // Store the label in a map since FunctionDeclarationNode doesn't have asmjitLabel member
    functionLabels[funcDecl] = funcLabel;

    std::cout << "Created label for function: " << funcDecl->name << std::endl;
}

void CodeGenerator::generateFunctionPrologue(FunctionDeclarationNode* funcDecl) {
    std::cout << "Generating prologue for function: " << funcDecl->funcName << std::endl;
    
    // Standard function prologue
    cb->push(x86::rbp);
    cb->mov(x86::rbp, x86::rsp);
    
    // Preserve callee-saved registers we use for scope management
    cb->push(x86::r14);
    cb->push(x86::r15);
    
    // Special case: main function needs to allocate its own scope since it's not called via our convention
    if (funcDecl->funcName == "main") {
        std::cout << "Main function - allocating scope in prologue" << std::endl;
        std::cout << "DEBUG: Main function totalSize before allocation: " << funcDecl->totalSize << std::endl;

        // Ensure the main function has a valid scope size
        if (funcDecl->totalSize == 0) {
            funcDecl->totalSize = 16; // Minimum scope size for metadata and flags
            std::cout << "DEBUG: Set main function totalSize to 16" << std::endl;
        }

        // Initialize r14 and r15 to nullptr for main (no parent scope)
        cb->xor_(x86::r14, x86::r14);
        cb->xor_(x86::r15, x86::r15);

        // Allocate scope for main
        allocateScope(funcDecl);
        
        // Main has no parameters, so nothing to copy
        std::cout << "Main scope allocated" << std::endl;
    } else {
        // NOTE: For regular functions, scope allocation and parameter copying now happens at the call site!
        // The function receives r15 already pointing to an allocated scope with parameters populated.
        // r14 points to the parent scope.
        // We don't need to save any registers or copy parameters here.
        
        std::cout << "Function prologue complete - scope already allocated by caller" << std::endl;
    }
}

void CodeGenerator::generateFunctionEpilogue(FunctionDeclarationNode* funcDecl) {
    std::cout << "Generating epilogue for function: " << funcDecl->funcName << std::endl;
    
    // Use generic scope epilogue to free scope and restore parent scope pointer
    generateScopeEpilogue(funcDecl);
    
    // Restore preserved callee-saved registers
    cb->pop(x86::r15);
    cb->pop(x86::r14);
    
    // Standard function epilogue
    cb->mov(x86::rsp, x86::rbp);
    cb->pop(x86::rbp);
    cb->ret();
}

void CodeGenerator::storeFunctionAddressInClosure(FunctionDeclarationNode* funcDecl, LexicalScopeNode* scope) {
    // Find the closure variable for this function in the current scope
    auto it = scope->variables.find(funcDecl->funcName);
    if (it == scope->variables.end() || it->second.type != DataType::CLOSURE) {
        return; // Function not referenced as closure in this scope
    }
    
    std::cout << "Storing function address for closure: " << funcDecl->funcName << std::endl;
    
    // Get the label for this function
    Label* funcLabel = static_cast<Label*>(funcDecl->asmjitLabel);
    if (!funcLabel) {
        throw std::runtime_error("Function label not created for: " + funcDecl->funcName);
    }
    
    // Get the address of the label by using lea with a RIP-relative reference
    // This creates a memory operand that references the label
    cb->lea(x86::rax, x86::ptr(*funcLabel));
    
    // Store the function address in the closure at the variable's offset
    int offset = it->second.offset;
    cb->mov(x86::ptr(x86::r15, offset), x86::rax);
    
    // Store the closure size (in bytes) right after the function address
    size_t closureSize = it->second.size;
    cb->mov(x86::rax, closureSize);
    cb->mov(x86::ptr(x86::r15, offset + 8), x86::rax);

    // now store any needed closure addresses for parent scopes allNeeded
    // loop through allNeeded with index to calculate proper offset
    int scopeIndex = 0;
    for (const auto& neededDepth : funcDecl->allNeeded) {
        int scopeOffset = offset + 16 + (scopeIndex * 8); // function_address (8) + size (8) + scope_pointers
        bool handled = false;

        // Special case: if we're in the scope that matches the needed depth,
        // we don't need to look up parameter mapping - use current scope directly
        if (scope->depth == neededDepth) {
            cb->mov(x86::rax, x86::r15); // current scope address
            handled = true;
        } else {
            // find parameter index in scopeDepthToParentParameterIndexMap
            auto mapIt = scope->scopeDepthToParentParameterIndexMap.find(neededDepth);
            if (mapIt == scope->scopeDepthToParentParameterIndexMap.end()) {
                throw std::runtime_error("Needed variable not found in scope: " + std::to_string(neededDepth));
            }

            int paramIndex = mapIt->second;
            if (paramIndex == -1) {
                // Immediate parent scope lives in r14
                cb->mov(x86::rax, x86::r14);
                handled = true;
            } else if (auto funcDeclParent = dynamic_cast<FunctionDeclarationNode*>(scope)) {
                // Function scope: parent pointers are stored in hidden parameters
                int hiddenParamIndex = paramIndex - static_cast<int>(funcDeclParent->paramsInfo.size());
                if (hiddenParamIndex < 0 || hiddenParamIndex >= static_cast<int>(funcDeclParent->hiddenParamsInfo.size())) {
                    throw std::runtime_error("Hidden parameter index out of range for needed variable: " + std::to_string(neededDepth));
                }
                int hiddenParamOffset = funcDeclParent->hiddenParamsInfo[hiddenParamIndex].offset;
                cb->mov(x86::rax, x86::ptr(x86::r15, hiddenParamOffset)); // load parent scope address from current scope's parameters
                handled = true;
            } else {
                // Block scope: parent pointers are stored sequentially after metadata
                int blockIndex = -1;
                for (size_t i = 0; i < scope->allNeeded.size(); ++i) {
                    if (scope->allNeeded[i] == neededDepth) {
                        blockIndex = static_cast<int>(i);
                        break;
                    }
                }
                if (blockIndex < 0) {
                    throw std::runtime_error("Block scope missing needed depth: " + std::to_string(neededDepth));
                }
                int parentPtrOffset = 8 + (blockIndex * 8);
                cb->mov(x86::rax, x86::ptr(x86::r15, parentPtrOffset));
                handled = true;
            }
        }

        if (!handled) {
            throw std::runtime_error("Failed to resolve scope pointer for closure: " + funcDecl->funcName);
        }

        cb->mov(x86::ptr(x86::r15, scopeOffset), x86::rax); // store in closure at proper offset
        scopeIndex++;
    }
}

// Generic scope management utilities that can be shared by functions and blocks
void CodeGenerator::generateScopePrologue(LexicalScopeNode* scope) {
    std::cout << "Generating scope prologue for scope at depth: " << scope->depth << std::endl;
    
    // Check if this is a function scope - functions are now handled at call site!
    if (dynamic_cast<FunctionDeclarationNode*>(scope)) {
        throw std::runtime_error("Function scopes should not use generateScopePrologue - scope allocated at call site!");
    }
    
    // This is a block scope - allocate and set up as before
    // Allocate the new scope (this will set r15 to point to the new scope)
    allocateScope(scope);
    
    // Copy needed parent scope addresses from current lexical environment
    std::cout << "Setting up block scope with access to " << scope->allNeeded.size() << " parent scopes" << std::endl;
    
    // For each needed parent scope depth, we need to find where to get the scope address from
    int scopeIndex = 0;
    for (const auto& neededDepth : scope->allNeeded) {
        // Calculate offset in the new scope where this parent scope address should be stored
        auto it = scope->scopeDepthToParentParameterIndexMap.find(neededDepth);
        if (it == scope->scopeDepthToParentParameterIndexMap.end()) {
            throw std::runtime_error("Needed parent scope not found in parameter mapping: " + std::to_string(neededDepth));
        }
        
        int paramIndex = it->second;
        // For blocks, we use a simple offset calculation since we don't have real parameters
        // Start at offset 8 to skip the flags field
        int offset = 8 + (scopeIndex * 8);
        
        std::cout << "  Parent scope at depth " << neededDepth << " -> block scope[" << offset << "]" << std::endl;

        std::cout << "    (paramIndex = " << paramIndex << ")" << std::endl;
        
        if (paramIndex == -1) {
            // Parent scope is current r14 (saved parent scope)
            cb->mov(x86::ptr(x86::r15, offset), x86::r14);
        } else {
            // Parent scope is stored in the current scope as a hidden parameter
            // Load it from the parent scope (r14) using proper parameter loading
            loadParameterIntoRegister(paramIndex, x86::rax, x86::r14);
            cb->mov(x86::ptr(x86::r15, offset), x86::rax); // Store in new scope
        }
        
        scopeIndex++;
    }

    // Create closures for any nested functions declared in this block scope
    for (const auto& [name, varInfo] : scope->variables) {
        if (varInfo.type == DataType::CLOSURE && varInfo.funcNode) {
            storeFunctionAddressInClosure(varInfo.funcNode, scope);
        }
    }
}

void CodeGenerator::generateScopeEpilogue(LexicalScopeNode* scope) {
    std::cout << "Generating scope epilogue for scope at depth: " << scope->depth << std::endl;
    
    // Pop scope from GC roots - this removes it from the active scope stack
    // but does NOT free the memory. The GC will handle scope destruction later.
    uint64_t gcPopScopeAddr = reinterpret_cast<uint64_t>(&gc_pop_scope);
    cb->mov(x86::rax, gcPopScopeAddr);
    cb->call(x86::rax);
    
    // DO NOT call free() here! Scopes should only be destroyed by the garbage collector.
    // The scope is now out of scope (removed from GC roots) but the memory remains
    // until the GC determines it's safe to free.
    
    // Restore r15 to the parent scope (from r14)
    cb->mov(x86::r15, x86::r14);
    
    // Restore r14 from the stack (the old parent)
    cb->pop(x86::r14);
}

void CodeGenerator::generateBlockStmt(BlockStatement* blockStmt) {
    std::cout << "Generating block statement" << std::endl;
    
    // Generate the scope prologue (allocate new scope, copy parent scope addresses)
    generateScopePrologue(blockStmt);
    
    // Update current scope for variable resolution
    LexicalScopeNode* prevScope = currentScope;
    currentScope = blockStmt;
    
    // Generate code for all statements in the block
    for (auto& child : blockStmt->children) {
        visitNode(child);
    }
    
    // Restore previous scope
    currentScope = prevScope;
    
    // Generate the scope epilogue (free scope, restore parent scope pointer)
    generateScopeEpilogue(blockStmt);
}

void CodeGenerator::generateFunctionCall(MethodCallNode* funcCall) {
    std::cout << "Generating function call: " << funcCall->value << std::endl;
    
    // Check if this is actually a method call
    MethodCallNode* methodCall = dynamic_cast<MethodCallNode*>(funcCall);
    bool isMethodCall = (methodCall != nullptr);
    
    if (isMethodCall) {
        std::cout << "  -> This is a method call on object" << std::endl;
    }
    
    // Get target function information
    FunctionDeclarationNode* targetFunc;
    
    if (isMethodCall) {
        targetFunc = methodCall->resolvedMethod;
    } else {
        targetFunc = funcCall->varRef->funcNode;
    }
    
    if (!targetFunc) {
        throw std::runtime_error("Cannot resolve target function for call: " + funcCall->value);
    }
    
    std::cout << "Target function has " << targetFunc->paramsInfo.size() << " regular params and " 
              << targetFunc->hiddenParamsInfo.size() << " hidden params" << std::endl;
    
    // Allocate scope for the callee
    // This will:
    // - Push r14 (save grandparent scope)
    // - Set r14 = r15 (current scope becomes parent of new scope)
    // - Allocate new scope memory
    // - Set r15 = new scope
    allocateScope(targetFunc);
    
    // Now r15 points to the new (child) scope, r14 points to caller's scope
    // We need to copy parameters from r14 (caller scope) to r15 (child scope)
    
    // Copy regular parameters into the child scope
    if (isMethodCall) {
        // First parameter is "this"
        const VariableInfo& thisParam = targetFunc->paramsInfo[0];
        std::cout << "  Copying 'this' to scope[" << thisParam.offset << "]" << std::endl;
        loadValue(methodCall->object, x86::rax, x86::r14, DataType::OBJECT);
        cb->mov(x86::ptr(x86::r15, thisParam.offset), x86::rax);
        
        // Then copy explicit method arguments
        for (size_t i = 0; i < methodCall->args.size(); i++) {
            const VariableInfo& param = targetFunc->paramsInfo[i + 1];
            ASTNode* arg = methodCall->args[i];
            
            std::cout << "  Copying method arg " << (i + 1) << " to scope[" << param.offset << "]" << std::endl;
            
            if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                // Load from caller's scope (r14)
                loadVariableFromScope(static_cast<IdentifierExpressionNode*>(arg), x86::rax, 0, x86::r14);
            } else {
                loadValue(arg, x86::rax, x86::r14, param.type);
            }
            cb->mov(x86::ptr(x86::r15, param.offset), x86::rax);
        }
    } else {
        // Regular function call - copy all arguments
        for (size_t i = 0; i < funcCall->args.size(); i++) {
            const VariableInfo& param = targetFunc->paramsInfo[i];
            ASTNode* arg = funcCall->args[i];
            
            std::cout << "  Copying regular arg " << i << " (" << param.name << ") to scope[" << param.offset << "]" << std::endl;
            
            if (arg->nodeType == ASTNodeType::IDENTIFIER_EXPRESSION) {
                // Load from caller's scope (r14)
                loadVariableFromScope(static_cast<IdentifierExpressionNode*>(arg), x86::rax, 0, x86::r14);
            } else {
                loadValue(arg, x86::rax, x86::r14, param.type);
            }
            cb->mov(x86::ptr(x86::r15, param.offset), x86::rax);
        }
    }
    
    // Load the closure address for accessing hidden parameters (parent scope pointers)
    // For method calls, load from object; for regular calls, load from variable
    if (isMethodCall) {
        // Load object pointer into rbx
        loadValue(methodCall->object, x86::rbx, x86::r14, DataType::OBJECT);
        // Add offset to get to the method closure pointer in the object
        int closurePtrOffset = ObjectLayout::HEADER_SIZE + methodCall->methodClosureOffset;
        std::cout << "  Loading method closure pointer from object at offset " << closurePtrOffset << std::endl;
        // Load the closure pointer (not add offset to object, but load the pointer value)
        cb->mov(x86::rbx, x86::qword_ptr(x86::rbx, closurePtrOffset));
    } else {
        // Load closure address from caller's scope (r14)
        // For regular function calls, the closure is stored in a variable
        if (funcCall->varRef) {
            // Create a temporary identifier to load the closure variable
            IdentifierExpressionNode tempId(nullptr, funcCall->value);
            tempId.varRef = funcCall->varRef;
            loadVariableAddress(&tempId, x86::rbx, 0, x86::r14);
        } else {
            throw std::runtime_error("Function call has no variable reference for closure: " + funcCall->value);
        }
    }
    
    // Copy hidden parameters (parent scope pointers) from closure into child scope
    for (size_t i = 0; i < targetFunc->hiddenParamsInfo.size(); i++) {
        const ParameterInfo& hiddenParam = targetFunc->hiddenParamsInfo[i];
        int closureOffset = 16 + (i * 8); // function_address (8) + size (8) + scope_pointers
        
        std::cout << "  Copying hidden param " << i << " (depth " << hiddenParam.depth 
                  << ") to scope[" << hiddenParam.offset << "]" << std::endl;
        
        // Load the scope pointer from closure and store in child scope
        cb->mov(x86::rax, x86::ptr(x86::rbx, closureOffset));
        cb->mov(x86::ptr(x86::r15, hiddenParam.offset), x86::rax);
    }
    
    // Load the function address from closure
    // For both method and regular calls, rbx points to the closure
    // Closure layout: [size(8)][func_addr(8)][scope_ptr1(8)]...[scope_ptrN(8)]
    cb->mov(x86::rax, x86::ptr(x86::rbx, 8)); // Load function address from closure (offset 8, after size)
    
    // Make the call - r15 already points to the pre-allocated and populated scope
    // The callee will use this scope directly
    cb->call(x86::rax);
    
    // After call returns, the callee's epilogue has already:
    // - Freed its scope (called gc_pop_scope)
    // - Done: mov r15, r14 (restored parent scope, which is our scope)
    // - Done: pop r14 (restored grandparent scope pointer)
    // So now r15 points back to our scope and r14 is restored
    
    std::cout << "Function call complete" << std::endl;
}



void CodeGenerator::generateNewExpr(NewExprNode* newExpr, x86::Gp destReg, x86::Gp sourceScopeReg) {
    std::cout << "Generating new expression for class: " << newExpr->className << std::endl;

    if (newExpr->isRawMemory) {
        if (newExpr->args.size() != 1) {
            throw std::runtime_error("RawMemory allocation expects exactly one size argument");
        }

        // Load size argument into rsi (second argument for calloc)
        loadValue(newExpr->args[0], x86::rsi, sourceScopeReg, DataType::INT64);

        // Set number of elements to 1 (first argument for calloc)
        cb->mov(x86::rdi, 1);

        // Call calloc wrapper to allocate zeroed memory
        uint64_t callocAddr = reinterpret_cast<uint64_t>(&calloc_wrapper);
        cb->mov(x86::rax, callocAddr);
        cb->call(x86::rax);

        // Result pointer returned in rax
        if (destReg.id() != x86::rax.id()) {
            cb->mov(destReg, x86::rax);
        }

        std::cout << "Generated RawMemory allocation - pointer returned" << std::endl;
        return;
    }
    
    // Verify that the class reference was set during analysis
    if (!newExpr->classRef) {
        throw std::runtime_error("Class reference not set for new expression: " + newExpr->className);
    }
    
    ClassDeclarationNode* classDecl = newExpr->classRef;
    
    // Calculate total object size: header + packed fields (includes closure pointers + regular fields)
    int totalObjectSize = ObjectLayout::HEADER_SIZE + classDecl->totalSize;
    
    std::cout << "DEBUG generateNewExpr: Allocating object of size " << totalObjectSize 
              << " (header=" << ObjectLayout::HEADER_SIZE 
              << ", packed fields=" << classDecl->totalSize << ")" << std::endl;
    
    // Call calloc to allocate and zero-initialize object
    // mov rdi, 1 (number of elements)
    cb->mov(x86::rdi, 1);
    // mov rsi, totalObjectSize (size of each element)
    cb->mov(x86::rsi, totalObjectSize);
    
    // Call calloc
    uint64_t callocAddr = reinterpret_cast<uint64_t>(&calloc_wrapper);
    cb->mov(x86::rax, callocAddr);
    cb->call(x86::rax);
    
    // Object pointer is now in rax
    // We need to initialize the object header
    
    // Get class metadata from registry and store at offset 0
    ClassMetadata* metadata = MetadataRegistry::getInstance().getClassMetadata(classDecl->className);
    if (!metadata) {
        throw std::runtime_error("Class metadata not found for: " + classDecl->className);
    }
    uint64_t metadataAddr = reinterpret_cast<uint64_t>(metadata);
    cb->mov(x86::r10, metadataAddr);
    cb->mov(x86::qword_ptr(x86::rax, ObjectLayout::METADATA_OFFSET), x86::r10);
    
    // Store flags at offset 8 (currently 0, zero-initialized by calloc)
    // cb->mov(x86::qword_ptr(x86::rax, ObjectLayout::FLAGS_OFFSET), 0);  // Not needed, calloc already zeroed
    
    // Initialize closure pointers in the object
    // Set closure pointers using the packed offsets
    for (size_t i = 0; i < classDecl->methodLayout.size(); i++) {
        const auto& methodInfo = classDecl->methodLayout[i];
        int closurePtrOffset = ObjectLayout::HEADER_SIZE + methodInfo.closureOffsetInObject;
        
        std::cout << "DEBUG generateNewExpr: Setting closure pointer " << i 
                  << " ('" << methodInfo.methodName << "') at object offset " << closurePtrOffset << std::endl;
        
        // Get the closure from metadata's simple array
        Closure* metadataClosure = metadata->methodClosures[i];
        if (!metadataClosure) {
            throw std::runtime_error("Failed to get closure for method: " + methodInfo.methodName);
        }
        
        uint64_t closureAddr = reinterpret_cast<uint64_t>(metadataClosure);
        
        // Store the closure pointer in the object
        cb->push(x86::rax); // Save object pointer
        cb->mov(x86::r10, closureAddr);
        cb->mov(x86::qword_ptr(x86::rax, closurePtrOffset), x86::r10);
        cb->pop(x86::rax); // Restore object pointer
    }
    
    std::cout << "DEBUG generateNewExpr: Object allocated at runtime, class metadata stored at offset " 
              << ObjectLayout::METADATA_OFFSET << std::endl;
    
    // Track object in GC (save rax first since it contains the object pointer)
    cb->push(x86::rax);
    cb->mov(x86::rdi, x86::rax);  // First argument: object pointer
    uint64_t gcTrackAddr = reinterpret_cast<uint64_t>(&gc_track_object);
    cb->mov(x86::r11, gcTrackAddr);
    cb->call(x86::r11);
    cb->pop(x86::rax);  // Restore object pointer
    
    // Move result to destination register if different
    if (destReg.id() != x86::rax.id()) {
        cb->mov(destReg, x86::rax);
    }
    
    std::cout << "Generated new expression - object pointer returned" << std::endl;
}

void CodeGenerator::generateRawMemoryRelease(MethodCallNode* methodCall) {
    std::cout << "Generating RawMemory release call" << std::endl;

    if (!methodCall->args.empty()) {
        throw std::runtime_error("RawMemory.release() does not take arguments");
    }

    // Load the raw memory pointer into rdi (first argument for free)
    loadValue(methodCall->object, x86::rdi, x86::r15, DataType::RAW_MEMORY);

    uint64_t freeAddr = reinterpret_cast<uint64_t>(&free_wrapper);
    cb->mov(x86::rax, freeAddr);
    cb->call(x86::rax);

    std::cout << "Emitted RawMemory release call" << std::endl;
}

bool CodeGenerator::isRawMemoryReleaseCall(MethodCallNode* methodCall) const {
    if (!methodCall) {
        return false;
    }
    if (methodCall->methodName != "release") {
        return false;
    }
    if (methodCall->args.size() != 0) {
        return false;
    }
    if (!methodCall->object) {
        return false;
    }

    ASTNode* objectNode = methodCall->object;

    switch (objectNode->type) {
        case ASTNodeType::IDENTIFIER_EXPRESSION: {
            auto identifier = static_cast<IdentifierExpressionNode*>(objectNode);
            return identifier->varRef && identifier->varRef->type == DataType::RAW_MEMORY;
        }
        case ASTNodeType::NEW_EXPR: {
            auto newExpr = static_cast<NewExprNode*>(objectNode);
            return newExpr->isRawMemory;
        }
        case ASTNodeType::MEMBER_ACCESS: {
            auto memberAccess = static_cast<MemberAccessNode*>(objectNode);
            if (!memberAccess->classRef) {
                return false;
            }
            auto fieldIt = memberAccess->classRef->fields.find(memberAccess->memberName);
            return fieldIt != memberAccess->classRef->fields.end() &&
                   fieldIt->second.type == DataType::RAW_MEMORY;
        }
        case ASTNodeType::METHOD_CALL: {
            auto methodCall = static_cast<MethodCallNode*>(objectNode);
            return methodCall->varRef && methodCall->varRef->type == DataType::RAW_MEMORY;
        }
        default:
            return false;
    }
}

void CodeGenerator::generateMemberAccess(MemberAccessNode* memberAccess, x86::Gp destReg) {
    std::cout << "Generating member access for member: " << memberAccess->memberName << std::endl;
    
    // Verify that the class reference and member offset were set during analysis
    if (!memberAccess->classRef) {
        throw std::runtime_error("Class reference not set for member access: " + memberAccess->memberName);
    }
    
    std::cout << "DEBUG generateMemberAccess: Accessing member '" << memberAccess->memberName 
              << "' at offset " << memberAccess->memberOffset << " in class '" 
              << memberAccess->classRef->className << "'" << std::endl;
    
    // Load the object pointer into a temporary register
    // The object could be an identifier (variable) or another expression
    x86::Gp objectPtrReg = x86::r10;
    loadValue(memberAccess->object, objectPtrReg);
    
    // Use the pre-calculated absolute offset (already includes header)
    int actualOffset = memberAccess->memberOffset;
    
    std::cout << "DEBUG generateMemberAccess: Loading from object pointer + " << actualOffset 
              << " (absolute offset)" << std::endl;
    
    // Load the field value from [objectPtrReg + actualOffset] into destReg
    cb->mov(destReg, x86::qword_ptr(objectPtrReg, actualOffset));
    
    std::cout << "Generated member access - field value loaded" << std::endl;
}

void CodeGenerator::generateMemberAssign(MemberAssignNode* memberAssign) {
    std::cout << "Generating member assignment" << std::endl;
    
    if (!memberAssign->member) {
        throw std::runtime_error("Member assignment has no member access node");
    }
    
    MemberAccessNode* member = memberAssign->member;
    
    // Verify that the class reference and member offset were set during analysis
    if (!member->classRef) {
        throw std::runtime_error("Class reference not set for member assignment: " + member->memberName);
    }
    
    std::cout << "DEBUG generateMemberAssign: Assigning to member '" << member->memberName 
              << "' at offset " << member->memberOffset << " in class '" 
              << member->classRef->className << "'" << std::endl;
    
    // Load the object pointer into a temporary register
    x86::Gp objectPtrReg = x86::r10;
    loadValue(member->object, objectPtrReg);
    
    DataType fieldType = DataType::INT64;
    if (member->classRef) {
        auto fieldIt = member->classRef->fields.find(member->memberName);
        if (fieldIt != member->classRef->fields.end()) {
            fieldType = fieldIt->second.type;
        }
    }
    
    // Use the pre-calculated absolute offset (already includes header)
    int actualOffset = member->memberOffset;
    
    if (fieldType == DataType::ANY) {
        loadAnyValue(memberAssign->value, x86::rax, x86::rdx);
        cb->mov(x86::qword_ptr(objectPtrReg, actualOffset), x86::rdx);
        cb->mov(x86::qword_ptr(objectPtrReg, actualOffset + 8), x86::rax);
        
        if (memberAssign->value->nodeType != ASTNodeType::NEW_EXPR) {
            Label skipObjectBarrier = cb->newLabel();
            cb->cmp(x86::rdx, static_cast<uint32_t>(DataType::OBJECT));
            cb->jne(skipObjectBarrier);

            cb->mov(x86::rcx, x86::qword_ptr(x86::rax, ObjectLayout::FLAGS_OFFSET));
            cb->test(x86::rcx, ObjectFlags::NEEDS_SET_FLAG);

            Label skipWriteBarrier = cb->newLabel();
            cb->jz(skipWriteBarrier);

            cb->or_(x86::qword_ptr(x86::rax, ObjectLayout::FLAGS_OFFSET), ObjectFlags::SET_FLAG);
            cb->mfence();

            cb->bind(skipWriteBarrier);
            cb->bind(skipObjectBarrier);
        }
        
        std::cout << "Generated member assignment for ANY field" << std::endl;
        return;
    }
    
    // Load the value to assign into another register
    x86::Gp valueReg = x86::rax;
    loadValue(memberAssign->value, valueReg, x86::r15, fieldType);
    
    std::cout << "DEBUG generateMemberAssign: Storing to object pointer + " << actualOffset 
              << " (absolute offset)" << std::endl;
    
    // Store the value to [objectPtrReg + actualOffset]
    cb->mov(x86::qword_ptr(objectPtrReg, actualOffset), valueReg);
    
    // Check if we're assigning an object reference
    // We need to check the field type in the class definition
    if (member->classRef) {
        auto it = member->classRef->fields.find(member->memberName);
        if (it != member->classRef->fields.end() && it->second.type == DataType::OBJECT) {
            // Skip write barrier for NEW expressions - they can't be suspected dead yet
            if (memberAssign->value->nodeType != ASTNodeType::NEW_EXPR) {
                // Inline GC write barrier - check needs_set_flag and atomically set set_flag if needed
                // This is much faster than a function call
                
                // Load flags from object header (at offset 8)
                // flags is at [valueReg + 8]
                cb->mov(x86::rcx, x86::qword_ptr(valueReg, ObjectLayout::FLAGS_OFFSET));  // Load flags
                
                // Test if NEEDS_SET_FLAG (bit 0) is set
                cb->test(x86::rcx, ObjectFlags::NEEDS_SET_FLAG);
                
                // Create a label for skipping if not needed
                Label skipWriteBarrier = cb->newLabel();
                cb->jz(skipWriteBarrier);  // Jump if zero (NEEDS_SET_FLAG not set)
                
                // NEEDS_SET_FLAG is set, so OR the SET_FLAG bit (no LOCK needed - idempotent)
                // Using non-locked OR is safe because we're only setting a bit to 1
                cb->or_(x86::qword_ptr(valueReg, ObjectLayout::FLAGS_OFFSET), ObjectFlags::SET_FLAG);
                
                // Memory fence to ensure write visibility to GC thread
                // This guarantees the set_flag write is visible before GC reads it in phase 3
                cb->mfence();
                
                cb->bind(skipWriteBarrier);
            }
        }
    }
    
    std::cout << "Generated member assignment - field value stored" << std::endl;
}

void CodeGenerator::generateClassDecl(ClassDeclarationNode* classDecl) {
    std::cout << "Generating class declaration (inline closure creation): " << classDecl->className << std::endl;
    
    // Get the runtime metadata for this class
    ClassMetadata* metadata = MetadataRegistry::getInstance().getClassMetadata(classDecl->className);
    if (!metadata) {
        throw std::runtime_error("Class metadata not found for: " + classDecl->className);
    }
    
    // The method code should already be generated from the function registry
    // Here we just need to ensure the metadata closures are set up
    // The actual patching of function addresses happens in patchMetadataClosures after code commit
    
    std::cout << "Class " << classDecl->className << " has " << classDecl->methodLayout.size() 
              << " methods (code already generated, closures will be patched)" << std::endl;
    
    for (size_t i = 0; i < classDecl->methodLayout.size(); i++) {
        auto& methodInfo = classDecl->methodLayout[i];
        auto& method = methodInfo.method;
        
        std::cout << "  Method: " << methodInfo.methodName << " - closure will be patched later" << std::endl;
        
        // Verify the label exists
        Label* funcLabel = static_cast<Label*>(method->asmjitLabel);
        if (!funcLabel) {
            throw std::runtime_error("Method label not created for: " + methodInfo.methodName);
        }
        
        // The closure is already in the metadata, it will be patched after code commit
        Closure* closure = metadata->methodClosures[i];
        if (!closure) {
            throw std::runtime_error("Method closure not found in metadata for: " + methodInfo.methodName);
        }
    }
    
    std::cout << "Class declaration processing complete for: " << classDecl->className << std::endl;
}

// Assembly library wrapper methods for internal use
void CodeGenerator::makeSafeUnorderedList(x86::Gp addressReg, x86::Gp offsetReg, int32_t initialSize) {
    if (asmLibrary) {
        asmLibrary->makeSafeUnorderedList(addressReg, offsetReg, initialSize);
    }
}

void CodeGenerator::addToSafeList(x86::Gp addressReg, x86::Gp offsetReg, x86::Gp valueReg) {
    if (asmLibrary) {
        asmLibrary->addToSafeList(addressReg, offsetReg, valueReg);
    }
}

void CodeGenerator::removeFromSafeList(x86::Gp addressReg, x86::Gp offsetReg, x86::Gp indexReg) {
    if (asmLibrary) {
        asmLibrary->removeFromSafeList(addressReg, offsetReg, indexReg);
    }
}

void CodeGenerator::compactSafeList(x86::Gp addressReg, x86::Gp offsetReg) {
    if (asmLibrary) {
        asmLibrary->compactSafeList(addressReg, offsetReg);
    }
}
