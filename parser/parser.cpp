#include "parser.h"
#include <iostream>

int main() {
    parse("var x: int64 = 5;");
    std::cout << "done\n";
}
