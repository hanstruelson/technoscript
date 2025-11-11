#include "parser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    std::cout << "Parsing file: " << filename << std::endl;
    std::cout << "Code length: " << code.length() << " characters" << std::endl;

    ASTNode* ast = parse(code);

    if (ast) {
        std::cout << "\nParse successful! AST:" << std::endl;
        printAst(ast, 0);
        // Clean up
        delete ast;
        return 0;
    } else {
        std::cout << "\nParse failed!" << std::endl;
        return 1;
    }
}
