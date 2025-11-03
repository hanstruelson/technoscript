#include "parser.h"
#include <iostream>

int main() {
    parse("function test() {}");
    std::cout << "done\n";
}
