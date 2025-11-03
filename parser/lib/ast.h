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
    UNION_TYPE,
    GENERIC_TYPE_PARAMETERS,
    GENERIC_TYPE,
    EXPRESSION,
    BINARY_EXPRESSION,
    LITERAL_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    PLUS_PLUS_PREFIX_EXPRESSION,
    MINUS_MINUS_PREFIX_EXPRESSION,
    PLUS_PLUS_POSTFIX_EXPRESSION,
    MINUS_MINUS_POSTFIX_EXPRESSION,
    LOGICAL_NOT_EXPRESSION,
    UNARY_PLUS_EXPRESSION,
    UNARY_MINUS_EXPRESSION,
    BITWISE_NOT_EXPRESSION,
    PARENTHESIS_EXPRESSION,
    FUNCTION_DECLARATION,
    FUNCTION_EXPRESSION,
    ARROW_FUNCTION_EXPRESSION,
    PARAMETER_LIST,
    PARAMETER,
    ARRAY_LITERAL,
    OBJECT_LITERAL,
    PROPERTY,
    TEMPLATE_LITERAL,
    REGEXP_LITERAL,
    IF_STATEMENT,
    ELSE_CLAUSE,
    ELSE_IF_CLAUSE,
    WHILE_STATEMENT,
    DO_WHILE_STATEMENT,
    FOR_STATEMENT,
    SWITCH_STATEMENT,
    CASE_CLAUSE,
    DEFAULT_CLAUSE,
    TRY_STATEMENT,
    CATCH_CLAUSE,
    FINALLY_CLAUSE,
    BLOCK_STATEMENT,
    INTERFACE_DECLARATION,
    INTERFACE_METHOD,
    CLASS_DECLARATION,
    CLASS_PROPERTY,
    CLASS_METHOD
};

class ASTNode {
public:
    ASTNodeType nodeType;
    string value;
    vector<ASTNode*> children;
    ASTNode* parent;
    void (*childrenComplete)(ASTNode*);

    ASTNode(ASTNode* parent) : nodeType(ASTNodeType::AST_NODE), value(""), parent(parent) {}

    void addChild(ASTNode* child) {
        child->parent = this;
        children.push_back(child);
    }
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
    ASTNode* typeAnnotation;
    ASTNode* initializer;

    VariableDefinitionNode(ASTNode* parent, VariableDefinitionType vType)
        : ASTNode(parent), varType(vType), typeAnnotation(nullptr), initializer(nullptr) {
        nodeType = ASTNodeType::VARIABLE_DEFINITION;
    }

    ~VariableDefinitionNode() override = default;

    void print(std::ostream& os, int indent) const override {
        static const char* varMap[] = {"const","var","let"};
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "VariableDefinition(" << varMap[static_cast<int>(varType)] << "," << name;
        if (typeAnnotation) {
            os << ":";
            if (typeAnnotation->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(typeAnnotation);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else if (typeAnnotation->nodeType == ASTNodeType::UNION_TYPE) {
                os << "union";
            }
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

class MinusMinusPrefixExpressionNode : public ExpressionNode {
public:
    std::string identifier;

    MinusMinusPrefixExpressionNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::MINUS_MINUS_PREFIX_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad=[indent](){return string(indent*2,' ');};
        os<<pad()<<"MinusMinusPrefix("<<identifier<<")\n";
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

class ParameterNode : public ASTNode {
public:
    std::string name;
    TypeAnnotationNode* typeAnnotation;
    ASTNode* defaultValue;

    ParameterNode(ASTNode* parent) : ASTNode(parent), typeAnnotation(nullptr), defaultValue(nullptr) {
        nodeType = ASTNodeType::PARAMETER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "Parameter(" << name;
        if (typeAnnotation) {
            static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
            os << ":" << typeMap[static_cast<int>(typeAnnotation->dataType)];
        }
        os << ")\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class ParameterListNode : public ASTNode {
public:
    std::vector<ParameterNode*> parameters;

    ParameterListNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::PARAMETER_LIST;
    }

    void addParameter(ParameterNode* param) {
        parameters.push_back(param);
        children.push_back(param);
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ParameterList\n";
        for (auto param : parameters) if (param) param->print(os, indent + 1);
    }
};

class FunctionDeclarationNode : public ASTNode {
public:
    std::string name;
    ParameterListNode* parameters;
    TypeAnnotationNode* returnType;
    ASTNode* body;

    FunctionDeclarationNode(ASTNode* parent) : ASTNode(parent), parameters(nullptr), returnType(nullptr), body(nullptr) {
        nodeType = ASTNodeType::FUNCTION_DECLARATION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "FunctionDeclaration(" << name << ")\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) {
            os << pad() << "  ReturnType: ";
            static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
            os << typeMap[static_cast<int>(returnType->dataType)] << "\n";
        }
        if (body) body->print(os, indent + 1);
    }
};

class FunctionExpressionNode : public ExpressionNode {
public:
    ParameterListNode* parameters;
    TypeAnnotationNode* returnType;
    ASTNode* body;

    FunctionExpressionNode(ASTNode* parent) : ExpressionNode(parent), parameters(nullptr), returnType(nullptr), body(nullptr) {
        nodeType = ASTNodeType::FUNCTION_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "FunctionExpression\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) {
            os << pad() << "  ReturnType: ";
            static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
            os << typeMap[static_cast<int>(returnType->dataType)] << "\n";
        }
        if (body) body->print(os, indent + 1);
    }
};

class ArrowFunctionExpressionNode : public ExpressionNode {
public:
    ParameterListNode* parameters;
    TypeAnnotationNode* returnType;
    ASTNode* body;

    ArrowFunctionExpressionNode(ASTNode* parent) : ExpressionNode(parent), parameters(nullptr), returnType(nullptr), body(nullptr) {
        nodeType = ASTNodeType::ARROW_FUNCTION_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ArrowFunctionExpression\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) {
            os << pad() << "  ReturnType: ";
            static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
            os << typeMap[static_cast<int>(returnType->dataType)] << "\n";
        }
        if (body) body->print(os, indent + 1);
    }
};

class ArrayLiteralNode : public ExpressionNode {
public:
    std::vector<ExpressionNode*> elements;

    ArrayLiteralNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::ARRAY_LITERAL;
    }

    void addElement(ExpressionNode* element) {
        elements.push_back(element);
        if (element) {
            element->parent = this;
            children.push_back(element);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ArrayLiteral\n";
        for (auto element : elements) if (element) element->print(os, indent + 1);
    }
};

class PropertyNode : public ASTNode {
public:
    std::string key;
    ExpressionNode* value;

    PropertyNode(ASTNode* parent) : ASTNode(parent), value(nullptr) {
        nodeType = ASTNodeType::PROPERTY;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "Property(" << key << ")\n";
        if (value) value->print(os, indent + 1);
    }
};

class ObjectLiteralNode : public ExpressionNode {
public:
    std::vector<PropertyNode*> properties;

    ObjectLiteralNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::OBJECT_LITERAL;
    }

    void addProperty(PropertyNode* property) {
        properties.push_back(property);
        if (property) {
            property->parent = this;
            children.push_back(property);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ObjectLiteral\n";
        for (auto property : properties) if (property) property->print(os, indent + 1);
    }
};

class BlockStatement : public ASTNode {
public:
    bool noBraces;

    BlockStatement(ASTNode* parent, bool noBraces = false) : ASTNode(parent), noBraces(noBraces) {
        nodeType = ASTNodeType::BLOCK_STATEMENT;
    }

    void addStatement(ASTNode* statement) {
        if (statement) {
            statement->parent = this;
            children.push_back(statement);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "BlockStatement";
        if (noBraces) os << "(noBraces)";
        os << "\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class ControlStatement : public ASTNode {
public:
    ControlStatement(ASTNode* parent, ASTNodeType type) : ASTNode(parent) {
        nodeType = type;
    }
};

class ElseClause : public ASTNode {
public:
    ElseClause(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::ELSE_CLAUSE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ElseClause\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class ElseIfClause : public ASTNode {
public:
    ElseIfClause(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::ELSE_IF_CLAUSE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ElseIfClause\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class IfStatement : public ControlStatement {
public:
    ExpressionNode* condition;

    IfStatement(ASTNode* parent) : ControlStatement(parent, ASTNodeType::IF_STATEMENT), condition(nullptr) {
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "IfStatement\n";
        for (auto child : children) {
            if (child == condition) {
                os << pad() << "  Condition:\n";
                child->print(os, indent + 2);
            } else if (child->nodeType == ASTNodeType::BLOCK_STATEMENT) {
                os << pad() << "  Consequent:\n";
                child->print(os, indent + 2);
            } else if (child->nodeType == ASTNodeType::ELSE_CLAUSE) {
                os << pad() << "  ElseClause:\n";
                child->print(os, indent + 2);
            } else {
                child->print(os, indent + 1);
            }
        }
    }
};

class WhileStatement : public ControlStatement {
public:
    ExpressionNode* condition;
    ASTNode* body;

    WhileStatement(ASTNode* parent) : ControlStatement(parent, ASTNodeType::WHILE_STATEMENT), condition(nullptr), body(nullptr) {
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "WhileStatement\n";
        if (condition) {
            os << pad() << "  Condition:\n";
            condition->print(os, indent + 2);
        }
        if (body) {
            os << pad() << "  Body:\n";
            body->print(os, indent + 2);
        }
    }
};

class InterfaceDeclarationNode : public ASTNode {
public:
    std::string name;
    std::vector<PropertyNode*> properties;
    std::vector<ASTNode*> methods;

    InterfaceDeclarationNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::INTERFACE_DECLARATION;
    }

    void addProperty(PropertyNode* property) {
        properties.push_back(property);
        if (property) {
            property->parent = this;
            children.push_back(property);
        }
    }

    void addMethod(ASTNode* method) {
        methods.push_back(method);
        if (method) {
            method->parent = this;
            children.push_back(method);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceDeclaration(" << name << ")\n";
        for (auto property : properties) if (property) property->print(os, indent + 1);
        for (auto method : methods) if (method) method->print(os, indent + 1);
    }
};

class ClassPropertyNode : public ASTNode {
public:
    std::string name;
    TypeAnnotationNode* typeAnnotation;
    ExpressionNode* initializer;
    bool isStatic;
    bool isReadonly;

    ClassPropertyNode(ASTNode* parent) : ASTNode(parent), typeAnnotation(nullptr), initializer(nullptr), isStatic(false), isReadonly(false) {
        nodeType = ASTNodeType::CLASS_PROPERTY;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassProperty(" << name;
        if (isStatic) os << ", static";
        if (isReadonly) os << ", readonly";
        os << ")\n";
        if (typeAnnotation) typeAnnotation->print(os, indent + 1);
        if (initializer) initializer->print(os, indent + 1);
    }
};

class ClassMethodNode : public ASTNode {
public:
    std::string name;
    ParameterListNode* parameters;
    TypeAnnotationNode* returnType;
    ASTNode* body;
    bool isStatic;

    ClassMethodNode(ASTNode* parent) : ASTNode(parent), parameters(nullptr), returnType(nullptr), body(nullptr), isStatic(false) {
        nodeType = ASTNodeType::CLASS_METHOD;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassMethod(" << name;
        if (isStatic) os << ", static";
        os << ")\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) returnType->print(os, indent + 1);
        if (body) body->print(os, indent + 1);
    }
};

class ClassDeclarationNode : public ASTNode {
public:
    std::string name;
    std::string extendsClass;
    std::vector<std::string> implementsInterfaces;
    std::vector<ClassPropertyNode*> properties;
    std::vector<ClassMethodNode*> methods;

    ClassDeclarationNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::CLASS_DECLARATION;
    }

    void addProperty(ClassPropertyNode* property) {
        properties.push_back(property);
        if (property) {
            property->parent = this;
            children.push_back(property);
        }
    }

    void addMethod(ClassMethodNode* method) {
        methods.push_back(method);
        if (method) {
            method->parent = this;
            children.push_back(method);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassDeclaration(" << name;
        if (!extendsClass.empty()) os << " extends " << extendsClass;
        if (!implementsInterfaces.empty()) {
            os << " implements ";
            for (size_t i = 0; i < implementsInterfaces.size(); ++i) {
                if (i > 0) os << ", ";
                os << implementsInterfaces[i];
            }
        }
        os << ")\n";
        for (auto property : properties) if (property) property->print(os, indent + 1);
        for (auto method : methods) if (method) method->print(os, indent + 1);
    }
};

class UnionTypeNode : public ASTNode {
public:
    std::vector<ASTNode*> types;

    UnionTypeNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::UNION_TYPE;
    }

    void addType(ASTNode* type) {
        types.push_back(type);
        if (type) {
            type->parent = this;
            children.push_back(type);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "UnionType\n";
        for (auto type : types) if (type) type->print(os, indent + 1);
    }
};

class GenericTypeParametersNode : public ASTNode {
public:
    std::vector<std::string> parameters;

    GenericTypeParametersNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::GENERIC_TYPE_PARAMETERS;
    }

    void addParameter(const std::string& param) {
        parameters.push_back(param);
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "GenericTypeParameters";
        if (!parameters.empty()) {
            os << "<";
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (i > 0) os << ", ";
                os << parameters[i];
            }
            os << ">";
        }
        os << "\n";
    }
};

class GenericTypeNode : public ASTNode {
public:
    std::string baseType;
    std::vector<ASTNode*> typeArguments;

    GenericTypeNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::GENERIC_TYPE;
    }

    void addTypeArgument(ASTNode* typeArg) {
        typeArguments.push_back(typeArg);
        if (typeArg) {
            typeArg->parent = this;
            children.push_back(typeArg);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "GenericType(" << baseType;
        if (!typeArguments.empty()) {
            os << "<";
            for (size_t i = 0; i < typeArguments.size(); ++i) {
                if (i > 0) os << ", ";
                // For now, just show the type name - could be improved to show full type
                if (typeArguments[i] && typeArguments[i]->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                    auto* typeNode = static_cast<TypeAnnotationNode*>(typeArguments[i]);
                    static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                    os << typeMap[static_cast<int>(typeNode->dataType)];
                } else {
                    os << "type";
                }
            }
            os << ">";
        }
        os << ")\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class PlusPlusPostfixExpressionNode : public ExpressionNode {
public:
    PlusPlusPostfixExpressionNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::PLUS_PLUS_POSTFIX_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "PlusPlusPostfix\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class MinusMinusPostfixExpressionNode : public ExpressionNode {
public:
    MinusMinusPostfixExpressionNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::MINUS_MINUS_POSTFIX_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "MinusMinusPostfix\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class LogicalNotExpressionNode : public ExpressionNode {
public:
    ExpressionNode* operand;

    LogicalNotExpressionNode(ASTNode* parent) : ExpressionNode(parent), operand(nullptr) {
        nodeType = ASTNodeType::LOGICAL_NOT_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "LogicalNot\n";
        if (operand) operand->print(os, indent + 1);
    }
};

class UnaryPlusExpressionNode : public ExpressionNode {
public:
    ExpressionNode* operand;

    UnaryPlusExpressionNode(ASTNode* parent) : ExpressionNode(parent), operand(nullptr) {
        nodeType = ASTNodeType::UNARY_PLUS_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "UnaryPlus\n";
        if (operand) operand->print(os, indent + 1);
    }
};

class UnaryMinusExpressionNode : public ExpressionNode {
public:
    ExpressionNode* operand;

    UnaryMinusExpressionNode(ASTNode* parent) : ExpressionNode(parent), operand(nullptr) {
        nodeType = ASTNodeType::UNARY_MINUS_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "UnaryMinus\n";
        if (operand) operand->print(os, indent + 1);
    }
};

class BitwiseNotExpressionNode : public ExpressionNode {
public:
    ExpressionNode* operand;

    BitwiseNotExpressionNode(ASTNode* parent) : ExpressionNode(parent), operand(nullptr) {
        nodeType = ASTNodeType::BITWISE_NOT_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "BitwiseNot\n";
        if (operand) operand->print(os, indent + 1);
    }
};

class TemplateLiteralNode : public ExpressionNode {
public:
    std::vector<std::string> quasis;
    std::vector<ExpressionNode*> expressions;

    TemplateLiteralNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::TEMPLATE_LITERAL;
    }

    void addQuasi(const std::string& quasi) {
        quasis.push_back(quasi);
    }

    void addExpression(ExpressionNode* expr) {
        expressions.push_back(expr);
        if (expr) {
            expr->parent = this;
            children.push_back(expr);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "TemplateLiteral\n";
        for (size_t i = 0; i < quasis.size(); ++i) {
            os << pad() << "  Quasi: \"" << quasis[i] << "\"\n";
            if (i < expressions.size() && expressions[i]) {
                os << pad() << "  Expression:\n";
                expressions[i]->print(os, indent + 2);
            }
        }
    }
};

class RegExpLiteralNode : public ExpressionNode {
public:
    std::string pattern;
    std::string flags;

    RegExpLiteralNode(ASTNode* parent, const std::string& pat, const std::string& flg = "")
        : ExpressionNode(parent), pattern(pat), flags(flg) {
        nodeType = ASTNodeType::REGEXP_LITERAL;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "RegExpLiteral(/ " << pattern << " /" << flags << ")\n";
    }
};

class DoWhileStatement : public ASTNode {
public:
    ASTNode* body;
    ExpressionNode* condition;

    DoWhileStatement(ASTNode* parent) : ASTNode(parent), body(nullptr), condition(nullptr) {
        nodeType = ASTNodeType::DO_WHILE_STATEMENT;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "DoWhileStatement\n";
        if (body) {
            os << pad() << "  Body:\n";
            body->print(os, indent + 2);
        }
        if (condition) {
            os << pad() << "  Condition:\n";
            condition->print(os, indent + 2);
        }
    }
};

class ForStatement : public ControlStatement {
public:
    ASTNode* init;
    ExpressionNode* test;
    ExpressionNode* update;
    ASTNode* body;

    ForStatement(ASTNode* parent) : ControlStatement(parent, ASTNodeType::FOR_STATEMENT), init(nullptr), test(nullptr), update(nullptr), body(nullptr) {
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ForStatement\n";
        if (init) {
            os << pad() << "  Init:\n";
            init->print(os, indent + 2);
        }
        if (test) {
            os << pad() << "  Test:\n";
            test->print(os, indent + 2);
        }
        if (update) {
            os << pad() << "  Update:\n";
            update->print(os, indent + 2);
        }
        if (body) {
            os << pad() << "  Body:\n";
            body->print(os, indent + 2);
        }
    }
};

class CaseClause : public ASTNode {
public:
    ExpressionNode* test;
    std::vector<ASTNode*> consequent;

    CaseClause(ASTNode* parent) : ASTNode(parent), test(nullptr) {
        nodeType = ASTNodeType::CASE_CLAUSE;
    }

    void addStatement(ASTNode* statement) {
        consequent.push_back(statement);
        if (statement) {
            statement->parent = this;
            children.push_back(statement);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        if (test) {
            os << pad() << "CaseClause\n";
            os << pad() << "  Test:\n";
            test->print(os, indent + 2);
        } else {
            os << pad() << "DefaultClause\n";
        }
        for (auto stmt : consequent) if (stmt) stmt->print(os, indent + 1);
    }
};

class SwitchStatement : public ASTNode {
public:
    ExpressionNode* discriminant;
    std::vector<CaseClause*> cases;

    SwitchStatement(ASTNode* parent) : ASTNode(parent), discriminant(nullptr) {
        nodeType = ASTNodeType::SWITCH_STATEMENT;
    }

    void addCase(CaseClause* caseClause) {
        cases.push_back(caseClause);
        if (caseClause) {
            caseClause->parent = this;
            children.push_back(caseClause);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "SwitchStatement\n";
        if (discriminant) {
            os << pad() << "  Discriminant:\n";
            discriminant->print(os, indent + 2);
        }
        for (auto caseClause : cases) if (caseClause) caseClause->print(os, indent + 1);
    }
};

class CatchClause : public ASTNode {
public:
    ParameterNode* param;
    BlockStatement* body;

    CatchClause(ASTNode* parent) : ASTNode(parent), param(nullptr), body(nullptr) {
        nodeType = ASTNodeType::CATCH_CLAUSE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "CatchClause\n";
        if (param) param->print(os, indent + 1);
        if (body) body->print(os, indent + 1);
    }
};

class FinallyClause : public ASTNode {
public:
    BlockStatement* body;

    FinallyClause(ASTNode* parent) : ASTNode(parent), body(nullptr) {
        nodeType = ASTNodeType::FINALLY_CLAUSE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "FinallyClause\n";
        if (body) body->print(os, indent + 1);
    }
};

class TryStatement : public ASTNode {
public:
    BlockStatement* block;
    CatchClause* handler;
    FinallyClause* finalizer;

    TryStatement(ASTNode* parent) : ASTNode(parent), block(nullptr), handler(nullptr), finalizer(nullptr) {
        nodeType = ASTNodeType::TRY_STATEMENT;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "TryStatement\n";
        if (block) block->print(os, indent + 1);
        if (handler) handler->print(os, indent + 1);
        if (finalizer) finalizer->print(os, indent + 1);
    }
};
