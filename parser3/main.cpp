#include <iostream>
#include "parser.h"

using namespace std;

int main() {
    cout << "hello world";
    auto parser = new Parser();
    string code = "test code";
    parser->parse(code);
}