#include "parser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <fstream>

int main(int argc, char* argv[]) {
    std::string code;
    if (argc > 1) {
        // Check if first argument is a file
        std::string firstArg = argv[1];
        if (firstArg.find('.') != std::string::npos || firstArg.find('/') != std::string::npos) {
            // Looks like a filename, try to read it
            std::ifstream file(firstArg);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    code += line + "\n";
                }
                file.close();
            } else {
                // Not a valid file, treat as code
                for (int i = 1; i < argc; ++i) {
                    if (i > 1) code += " ";
                    code += argv[i];
                }
            }
        } else {
            // Read from command line arguments
            for (int i = 1; i < argc; ++i) {
                if (i > 1) code += " ";
                code += argv[i];
            }
        }
    } else {
        // Read from stdin
        std::string line;
        while (std::getline(std::cin, line)) {
            code += line + "\n";
        }
    }

    if (!code.empty()) {
        parse(code);
    }
    return 0;
}
