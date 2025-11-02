CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -O0 -g -I.
# LDFLAGS = -lcapstone -lasmjit
# Updated sources after moving emitter functionality into codegen.cpp
SOURCES = parser.cpp
TARGET = parser

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TEST_TARGET) $(TEST_SOURCES)

clean:
	rm -f $(TARGET) $(TEST_TARGET)

.PHONY: clean
