#pragma once

#include "state.h";
#include <cstddef>
#include <string>

using namespace std;

class Parser {
public:
    string source;
    size_t currentIndex;
    unsigned char currentChar;
    State state;

    void parse(string &code) {
        currentChar = source[currentIndex];
        handleStateNone();
    }
    void handleStateNone() {
        if (isspace(currentChar))
    }
};