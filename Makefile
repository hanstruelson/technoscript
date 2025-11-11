CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wno-unused-parameter -O0 -g -I.
LDFLAGS = -lcapstone -lasmjit
# Updated sources after moving emitter functionality into codegen.cpp
SOURCES = main.cpp ast_printer.cpp codegen.cpp codegen_array.cpp library.cpp goroutine.cpp gc.cpp asm_library.cpp data_structures/safe_unordered_list.cpp parser/src/parser/parser.cpp
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = technoscript
TEST_TARGET = test_safe_unordered_list
TEST_SOURCES = tests/test_safe_unordered_list.cpp data_structures/safe_unordered_list.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) -o $(TEST_TARGET) $(TEST_OBJECTS)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(OBJECTS) $(TEST_OBJECTS)

.PHONY: clean test
