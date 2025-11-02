#pragma once
#include <algorithm>
#include <ostream>
#include <string>
#include <vector>

using namespace std;

struct ParserContext;

enum ASTNodeType {
    AST_NODE,
    VARIABLE_DEFINITION,
    TYPE_ANNOTATION,
    EXPRESSION,
    BINARY_EXPRESSION,
    LITERAL_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    PLUS_PLUS_PREFIX_EXPRESSION,
    PARENTHESIS_EXPRESSION
};

class ASTNode {
public:
    ASTNodeType nodeType;
    string value;
    vector<ASTNode*> children;
    ASTNode* parent;
    void (*childrenComplete)(ASTNode*);

    ASTNode(ASTNode* parent) : nodeType(ASTNodeType::AST_NODE), value(""), parent(parent) {}
    virtual ~ASTNode() {
        for (auto child : children) {
            delete child;
        }
    }

    virtual void print(std::ostream& os, int indent) const {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"ASTNode";
        if(!value.empty()) os<<"("<<value<<")";
        os<<"\n";
        for(auto child:children) if(child) child->print(os,indent+1);
    }
};

enum class VariableDefinitionType {
    CONST,
    VAR,
    LET
};

enum class DataType {
    INT64,
    FLOAT64,
    STRING,
    RAW_MEMORY,
    OBJECT,
};

class TypeAnnotationNode : public ASTNode {
public:
    DataType dataType;

    TypeAnnotationNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::TYPE_ANNOTATION;
    }

    void print(std::ostream& os, int indent) const override {
        static const char* map[]={"int64","float64","string","raw_memory","object"};
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"TypeAnnotation("<<map[static_cast<int>(dataType)]<<")\n";
        for(auto child:children) if(child) child->print(os,indent+1);
    }
};

class VariableDefinitionNode : public ASTNode {
public:
    string name;
    VariableDefinitionType varType;
    TypeAnnotationNode* typeAnnotation;
    ASTNode* initializer;

    VariableDefinitionNode(ASTNode* parent, VariableDefinitionType vType)
        : ASTNode(parent), varType(vType), typeAnnotation(nullptr), initializer(nullptr) {
        nodeType = ASTNodeType::VARIABLE_DEFINITION;
    }

    ~VariableDefinitionNode() override = default;

    void print(std::ostream& os, int indent) const override {
        static const char* varMap[] = {"const","var","let"};
        static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "VariableDefinition(" << varMap[static_cast<int>(varType)] << "," << name;
        if (typeAnnotation) {
            os << ":" << typeMap[static_cast<int>(typeAnnotation->dataType)];
        }
        os << ")\n";
        bool initializerPrinted = false;
        for (auto child : children) {
            if (!child) continue;
            child->print(os, indent + 1);
            if (child == initializer) initializerPrinted = true;
        }
        if (initializer && !initializerPrinted) {
            initializer->print(os, indent + 1);
        }
    }
};

class ExpressionNode : public ASTNode {
public:
    ExpressionNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"Expression()\n";
        for(auto child:children) if(child) child->print(os,indent+1);
    }

    virtual ~ExpressionNode() = default;
};

class LiteralExpressionNode : public ExpressionNode {
public:
    std::string literal;

    LiteralExpressionNode(ASTNode* parent, const std::string& value)
        : ExpressionNode(parent), literal(value) {
        this->value = value;
       nodeType = ASTNodeType::LITERAL_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"Literal"<<literal<<"\n";
    }
};

class IdentifierExpressionNode : public ExpressionNode {
public:
    std::string name;

    IdentifierExpressionNode(ASTNode* parent, const std::string& identifier)
        : ExpressionNode(parent), name(identifier) {
        this->value = identifier;
        nodeType = ASTNodeType::IDENTIFIER_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"Identifier("<<name<<")\n";
    }
};

class PlusPlusPrefixExpressionNode : public ExpressionNode {
public:
    std::string identifier;

    PlusPlusPrefixExpressionNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::PLUS_PLUS_PREFIX_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"PlusPlusPrefix("<<identifier<<")\n";
    }
};

enum BinaryExpressionOperator {
    OP_NULL,

    // Arithmetic
    OP_ADD,             // +
    OP_SUBTRACT,        // -
    OP_MULTIPLY,        // *
    OP_DIVIDE,          // /
    OP_MODULO,          // %

    // Exponentiation
    OP_EXPONENT,        // **

    // Bitwise
    OP_BIT_AND,         // &
    OP_BIT_OR,          // |
    OP_BIT_XOR,         // ^
    OP_LEFT_SHIFT,      // <<
    OP_RIGHT_SHIFT,     // >>
    OP_UNSIGNED_RIGHT_SHIFT, // >>>

    // Logical
    OP_AND,             // &&
    OP_OR,              // ||

    // Nullish coalescing
    OP_NULLISH_COALESCE, // ??

    // Comparison
    OP_EQUAL,           // ==
    OP_NOT_EQUAL,       // !=
    OP_STRICT_EQUAL,    // ===
    OP_STRICT_NOT_EQUAL,// !==
    OP_GREATER,         // >
    OP_GREATER_EQUAL,   // >=
    OP_LESS,            // <
    OP_LESS_EQUAL,      // <=

    // Other JS/TS binary operators
    OP_IN,              // in
    OP_INSTANCEOF,      // instanceof

    // Assignment (including compound)
    OP_ASSIGN,              // =
    OP_ADD_ASSIGN,          // +=
    OP_SUBTRACT_ASSIGN,     // -=
    OP_MULTIPLY_ASSIGN,     // *=
    OP_DIVIDE_ASSIGN,       // /=
    OP_MODULO_ASSIGN,       // %=
    OP_EXPONENT_ASSIGN,     // **=
    OP_LEFT_SHIFT_ASSIGN,   // <<=
    OP_RIGHT_SHIFT_ASSIGN,  // >>=
    OP_UNSIGNED_RIGHT_SHIFT_ASSIGN, // >>>=
    OP_BIT_AND_ASSIGN,      // &=
    OP_BIT_OR_ASSIGN,       // |=
    OP_BIT_XOR_ASSIGN,      // ^=
    OP_AND_ASSIGN,          // &&=
    OP_OR_ASSIGN,           // ||=
    OP_NULLISH_ASSIGN       // ??=
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryExpressionOperator op = BinaryExpressionOperator::OP_NULL;

    BinaryExpressionNode(ASTNode* parent, BinaryExpressionOperator op)
        : ExpressionNode(parent), op(op) {
        nodeType = ASTNodeType::BINARY_EXPRESSION;
        children.resize(2, nullptr);
    }

    void setLeft(ExpressionNode* node) {
        children[0] = node;
        if (node) node->parent = this;
    }

    void setRight(ExpressionNode* node) {
        children[1] = node;
        if (node) node->parent = this;
    }

    int operatorPrecedence(BinaryExpressionOperator op) {
        switch (op) {
            case BinaryExpressionOperator::OP_MULTIPLY:
            case BinaryExpressionOperator::OP_DIVIDE:
                return 2;
            case BinaryExpressionOperator::OP_ADD:
            case BinaryExpressionOperator::OP_SUBTRACT:
                return 1;
            default:
                return 0;
        }
    }

    bool isNewOperatorGreaterPrecedence(BinaryExpressionOperator newOp) {
        return operatorPrecedence(newOp) > operatorPrecedence(op);
    }

    ExpressionNode* left() const { return static_cast<ExpressionNode*>(children[0]); }
    ExpressionNode* right() const { return static_cast<ExpressionNode*>(children[1]); }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        auto opToString=[&](){
            switch(op){
                case BinaryExpressionOperator::OP_ADD: return "+";
                case BinaryExpressionOperator::OP_SUBTRACT: return "-";
                case BinaryExpressionOperator::OP_MULTIPLY: return "*";
                case BinaryExpressionOperator::OP_DIVIDE: return "/";
                case BinaryExpressionOperator::OP_MODULO: return "%";
                default: return "?";
            }
        };
        os << pad() << "BinaryExpression(" << opToString() << ")\n";
        if (left()) left()->print(os, indent + 1);
        if (right()) right()->print(os, indent + 1);
    }
};


// parenthesis expression node
class ParenthesisExpressionNode : public ExpressionNode {
public:
    ParenthesisExpressionNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::PARENTHESIS_EXPRESSION;
    }

    static void closeParenthesis(ParserContext& ctx);
};
