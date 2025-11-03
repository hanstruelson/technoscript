#include "parser.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <unistd.h>

int main(int argc, char* argv[]) {
    std::string code;
    if (argc > 1) {
        // Read from command line arguments
        for (int i = 1; i < argc; ++i) {
            if (i > 1) code += " ";
            code += argv[i];
        }
    } else {
        // Read from stdin only if it's not a terminal (to avoid hanging)
        if (!isatty(fileno(stdin))) {
            std::string line;
            while (std::getline(std::cin, line)) {
                code += line + "\n";
            }
        }
    }

    if (!code.empty()) {
        parse(code);
    }
    return 0;
}
