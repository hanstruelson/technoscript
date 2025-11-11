#include "parser/src/parser/parser.h"
#include "parser/src/analyzer.h"
#include "goroutine.h"
#include "gc.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "codegen.h"

int main(int argc, char* argv[]) {
    std::cout << "DEBUG: Program started" << std::endl;
    std::cout.flush();

    std::string code;
    if (argc > 1) {
        // Read code from file
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        code = buffer.str();
        file.close();
        std::cout << "DEBUG: Loaded code from file: " << argv[1] << std::endl;
    } else {
        std::cout << "DEBUG: Using built-in test program" << std::endl;
        code = R"(
var x: int64=42;
print(x)
)";
    }
    std::cout << "=== Testing safe unordered list ===\n";


    Analyzer analyzer;
    Codegen codeGen;

    std::cout << "DEBUG: Starting parsing..." << std::endl;
    ASTNode* ast = parse(code);
    std::cout << "DEBUG: Parsing completed successfully" << std::endl;
    std::cout << "DEBUG: Root node type: " << static_cast<int>(ast->nodeType) << std::endl;

    // Print AST for debugging
    std::cout << "\n=== AST ===" << std::endl;
    ast->print(std::cout, 0);
    std::cout << "=== END AST ===\n" << std::endl;

    std::cout << "DEBUG: Starting analysis..." << std::endl;
    analyzer.analyze(ast);
    std::cout << "DEBUG: Analysis completed successfully" << std::endl;

    // Get registries from analyzer
    auto classReg = analyzer.getClassRegistry();

    // Build class metadata registry (needed for GC tracing)
    std::cout << "DEBUG: Building class metadata registry..." << std::endl;
    MetadataRegistry::getInstance().buildClassMetadata(classReg);
    std::cout << "DEBUG: Class metadata registry built successfully" << std::endl;

    std::cout << "DEBUG: Starting code generation..." << std::endl;
    codeGen.generateProgram(*ast, classReg);
    std::cout << "DEBUG: Code generation completed successfully" << std::endl;

    // Directly run generated program for debugging
    std::cout << "\n=== Running program directly ===" << std::endl;
    codeGen.run();
    std::cout << "=== Program finished ===" << std::endl;

    return 0;
}
