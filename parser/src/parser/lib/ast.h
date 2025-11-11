#pragma once
#include <algorithm>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Include asmjit for compatibility
#include <asmjit/asmjit.h>

#include "parser_context.h"

using namespace std;



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
    CLOSURE,
    ANY,
};

enum class AccessModifier {
    PUBLIC,
    PRIVATE,
    PROTECTED,
    BLOCK
};

// Forward declarations for analysis structures
class FunctionDeclarationNode;
class LexicalScopeNode;
class ClassDeclarationNode;
class ScopeMetadata;

// Analysis structures (similar to old AST)
struct VariableInfo {
    DataType type;
    std::string name;
    int offset = 0;  // Offset from R15 where this variable/parameter is stored
    int size = 8;    // Size in bytes: 8 for regular vars, correct closure size for closures, calculated for objects
    LexicalScopeNode* definedIn = nullptr;
    FunctionDeclarationNode* funcNode = nullptr; // For closures: back-reference to function
    ClassDeclarationNode* classNode = nullptr; // For objects: pointer to class definition

    // Additional analysis fields for scoping
    VariableDefinitionType varType = VariableDefinitionType::VAR;
    LexicalScopeNode* definingScope = nullptr;
    int scopeDepth = 0;
    bool isDefined = true;
};

// Structure to track closure creation and patching
struct ClosurePatchInfo {
    int scopeOffset;              // Offset in scope where closure is stored
    FunctionDeclarationNode* function;   // Function this closure points to
    LexicalScopeNode* scope;      // Scope where this closure is created
};

// New structure for unified parameter information
struct ParameterInfo {
    int depth;                    // Scope depth
    int offset;                   // Byte offset in parameter area
    LexicalScopeNode* scope;      // Pointer to the actual scope
    bool isHiddenParam;           // true for lexical scopes, false for regular params

    ParameterInfo(int d = 0, int o = 0, LexicalScopeNode* s = nullptr, bool hidden = false)
        : depth(d), offset(o), scope(s), isHiddenParam(hidden) {}
};

struct ParserContext;

enum ASTNodeType {
    AST_NODE,
    VARIABLE_DEFINITION,
    TYPE_ANNOTATION,
    UNION_TYPE,
    INTERSECTION_TYPE,
    GENERIC_TYPE_PARAMETERS,
    GENERIC_TYPE,
    // Advanced generics
    CONDITIONAL_TYPE,
    INFER_TYPE,
    TEMPLATE_LITERAL_TYPE,
    MAPPED_TYPE,
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
    AWAIT_EXPRESSION,
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
    INTERFACE_PROPERTY,
    INTERFACE_INDEX_SIGNATURE,
    INTERFACE_CALL_SIGNATURE,
    INTERFACE_CONSTRUCT_SIGNATURE,
    CLASS_DECLARATION,
    CLASS_EXPRESSION,
    CLASS_PROPERTY,
    CLASS_METHOD,
    CLASS_GETTER,
    CLASS_SETTER,
    // Object operations
    MEMBER_ACCESS,
    MEMBER_ASSIGN,
    METHOD_CALL,
    THIS_EXPR,
    NEW_EXPR,
    // Module declarations
    IMPORT_DECLARATION,
    IMPORT_SPECIFIER,
    IMPORT_DEFAULT_SPECIFIER,
    IMPORT_NAMESPACE_SPECIFIER,
    EXPORT_NAMED_DECLARATION,
    EXPORT_SPECIFIER,
    EXPORT_DEFAULT_DECLARATION,
    EXPORT_ALL_DECLARATION,
    // Destructuring nodes
    ARRAY_DESTRUCTURING,
    OBJECT_DESTRUCTURING,
    // Enum nodes
    ENUM_DECLARATION,
    ENUM_MEMBER,
    // Type alias nodes
    TYPE_ALIAS,
    // Compatibility aliases
    IDENTIFIER = IDENTIFIER_EXPRESSION,
    FUNCTION_CALL = METHOD_CALL
};

class ASTNode;
class GenericTypeParametersNode;
class ParameterListNode;
class TypeAnnotationNode;

class ASTNode {
public:
    ASTNodeType nodeType;
    string value;
    vector<ASTNode*> children;
    ASTNode* parent;
    void (*childrenComplete)(ASTNode*);

    // Compatibility members for codegen
    ASTNodeType type; // Alias for nodeType

    ASTNode(ASTNode* parent) : nodeType(ASTNodeType::AST_NODE), value(""), parent(parent), type(ASTNodeType::AST_NODE) {}

    virtual void onBlockComplete(ParserContext& ctx) {
        // Default: do nothing
    }

    void addChild(ASTNode* child) {
        child->parent = this;
        children.push_back(child);
    }
    virtual ~ASTNode() {
        for (auto child : children) {
            delete child;
        }
    }

    virtual void walk() {
        std::cout << "walk" << std::endl;
        for (auto child : children) {
            if (child) child->walk();
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

// Forward declaration for ScopeMetadata
class ScopeMetadata;

// Base class for nodes that create lexical scopes
class LexicalScopeNode : public ASTNode {
public:
    bool isBlockScope;
    int depth = 0; // Analysis: scope depth
    LexicalScopeNode* parentFunctionScope = nullptr; // Analysis: parent scope
    std::set<int> parentDeps; // Analysis: parent scope depths this scope depends on
    std::set<int> descendantDeps; // Analysis: descendant scope depths needed by descendants
    std::vector<int> allNeeded; // Analysis: combined dependencies
    int totalSize = 0; // Analysis: total packed size of this scope
    std::map<std::string, VariableInfo> variables; // Analysis: variables in this scope

    // Compatibility members for codegen
    ScopeMetadata* metadata = nullptr;
    std::map<int, int> scopeDepthToParentParameterIndexMap;

    LexicalScopeNode(ASTNode* parent, bool isBlockScope)
        : ASTNode(parent), isBlockScope(isBlockScope) {}

    virtual void walk() override {
        std::cout << "walkLexicalScope" << (isBlockScope ? "Block" : "Function") << std::endl;
        for (auto child : children) {
            if (child) child->walk();
        }
    }

    void updateAllNeeded() {
        allNeeded.clear();
        for (int depthIdx : parentDeps) {
            allNeeded.push_back(depthIdx);
        }
        for (int depthIdx : descendantDeps) {
            if (parentDeps.find(depthIdx) == parentDeps.end()) {
                allNeeded.push_back(depthIdx);
            }
        }
    }
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
    ASTNode* pattern; // For destructuring patterns

    // Compatibility members for codegen
    bool isArray = false;
    std::string varName; // Alias for name
    DataType varType_compat = DataType::INT64;

    VariableDefinitionNode(ASTNode* parent, VariableDefinitionType vType)
        : ASTNode(parent), varType(vType), typeAnnotation(nullptr), initializer(nullptr), pattern(nullptr) {
        nodeType = ASTNodeType::VARIABLE_DEFINITION;
        varName = name;
        // Map VariableDefinitionType to DataType
        switch (vType) {
            case VariableDefinitionType::CONST:
            case VariableDefinitionType::VAR:
            case VariableDefinitionType::LET:
                varType_compat = DataType::INT64; // Default
                break;
        }
    }

    ~VariableDefinitionNode() override = default;

    void print(std::ostream& os, int indent) const override {
        static const char* varMap[] = {"const","var","let"};
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "VariableDefinition(" << varMap[static_cast<int>(varType)];
        if (!name.empty()) {
            os << "," << name;
        }
        if (typeAnnotation) {
            os << ":";
            if (typeAnnotation->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(typeAnnotation);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else if (typeAnnotation->nodeType == ASTNodeType::UNION_TYPE) {
                os << "union";
            } else if (typeAnnotation->nodeType == ASTNodeType::INTERSECTION_TYPE) {
                os << "intersection";
            }
        }
        os << ")\n";
        bool initializerPrinted = false;
        bool patternPrinted = false;
        for (auto child : children) {
            if (!child) continue;
            child->print(os, indent + 1);
            if (child == initializer) initializerPrinted = true;
            if (child == pattern) patternPrinted = true;
        }
        if (initializer && !initializerPrinted) {
            initializer->print(os, indent + 1);
        }
        if (pattern && !patternPrinted) {
            pattern->print(os, indent + 1);
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
    VariableInfo* varRef = nullptr; // Analysis: resolved variable reference
    LexicalScopeNode* accessedIn = nullptr; // Analysis: scope where this identifier is accessed

    // Compatibility members for codegen
    std::string value; // Alias for name

    IdentifierExpressionNode(ASTNode* parent, const std::string& identifier)
        : ExpressionNode(parent), name(identifier) {
        this->value = identifier;
        nodeType = ASTNodeType::IDENTIFIER_EXPRESSION;
    }

    // Add missing getVariableAccess method
    struct VariableAccess {
        bool inCurrentScope = false;
        int scopeParameterIndex = -1;
        int offset = 0;
    };

    VariableAccess getVariableAccess() {
        VariableAccess access;
        if (varRef) {
            access.inCurrentScope = (varRef->definedIn == nullptr); // Simplified
            access.offset = varRef->offset;
        }
        return access;
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
    ASTNode* pattern;  // Expression pattern (identifier, destructuring, rest, etc.)
    TypeAnnotationNode* typeAnnotation;
    ASTNode* defaultValue;

    ParameterNode(ASTNode* parent) : ASTNode(parent), pattern(nullptr), typeAnnotation(nullptr), defaultValue(nullptr) {
        nodeType = ASTNodeType::PARAMETER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "Parameter";
        if (typeAnnotation) {
            static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
            os << "(";
            if (typeAnnotation->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(typeAnnotation);
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
            os << ")";
        }
        os << "\n";
        if (pattern) {
            os << pad() << "  Pattern:\n";
            pattern->print(os, indent + 2);
        }
        if (defaultValue) {
            os << pad() << "  Default:\n";
            defaultValue->print(os, indent + 2);
        }
        for (auto child : children) if (child && child != pattern && child != defaultValue) child->print(os, indent + 1);
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



class FunctionDeclarationNode : public LexicalScopeNode {
public:
    std::string name;
    GenericTypeParametersNode* genericParameters;
    ParameterListNode* parameters;
    TypeAnnotationNode* returnType;
    ASTNode* body;
    bool isAsync;

    // Analysis: Unified parameter information
    std::vector<VariableInfo> paramsInfo;        // Regular parameters with calculated offsets
    std::vector<ParameterInfo> hiddenParamsInfo; // Hidden scope parameters with calculated offsets

    // Compatibility members for codegen
    asmjit::Label* asmjitLabel = nullptr;
    bool isMethod = false;
    ClassDeclarationNode* owningClass = nullptr;
    std::string funcName; // Alias for name
    ScopeMetadata* metadata = nullptr;

    FunctionDeclarationNode(ASTNode* parent) : LexicalScopeNode(parent, false), genericParameters(nullptr), parameters(nullptr), returnType(nullptr), body(nullptr), isAsync(false) {
        nodeType = ASTNodeType::FUNCTION_DECLARATION;
        funcName = name; // Sync with name
    }

    void walk() override {
        std::cout << "walkFunctionDeclaration" << std::endl;
        for (auto child : children) {
            if (child) child->walk();
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "FunctionDeclaration(" << name << ")\n";
        // TODO: Print generic parameters when class is fully defined
        // if (genericParameters) genericParameters->print(os, indent + 1);
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

class BlockStatement : public LexicalScopeNode {
public:
    bool noBraces;

    // Compatibility members for codegen
    ScopeMetadata* metadata = nullptr;

    BlockStatement(ASTNode* parent, bool noBraces = false) : LexicalScopeNode(parent, true), noBraces(noBraces) {
        nodeType = ASTNodeType::BLOCK_STATEMENT;
    }

    void addStatement(ASTNode* statement) {
        if (statement) {
            statement->parent = this;
            children.push_back(statement);
        }
    }

    void walk() override {
        std::cout << "walkBlockStatement" << std::endl;
        for (auto child : children) {
            if (child) child->walk();
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

    void onBlockComplete(ParserContext& ctx) override {
        ctx.state = STATE::IF_ALTERNATE_START;
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

    void walk() override {
        std::cout << "walkWhileStatement" << std::endl;
        for (auto child : children) {
            if (child) child->walk();
        }
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

class InterfacePropertyNode : public ASTNode {
public:
    std::string name;
    ASTNode* typeAnnotation;
    bool isOptional;
    bool isReadonly;

    InterfacePropertyNode(ASTNode* parent) : ASTNode(parent), typeAnnotation(nullptr), isOptional(false), isReadonly(false) {
        nodeType = ASTNodeType::INTERFACE_PROPERTY;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceProperty(" << name;
        if (isReadonly) os << ", readonly";
        if (isOptional) os << ", optional";
        os << ")\n";
        if (typeAnnotation) typeAnnotation->print(os, indent + 1);
    }
};

class InterfaceDeclarationNode : public ASTNode {
public:
    std::string name;
    GenericTypeParametersNode* genericParameters;
    std::vector<std::string> extendsInterfaces;
    std::vector<PropertyNode*> properties;
    std::vector<InterfacePropertyNode*> interfaceProperties;
    std::vector<ASTNode*> methods;
    std::vector<ASTNode*> indexSignatures;
    std::vector<ASTNode*> callSignatures;
    std::vector<ASTNode*> constructSignatures;

    InterfaceDeclarationNode(ASTNode* parent) : ASTNode(parent), genericParameters(nullptr) {
        nodeType = ASTNodeType::INTERFACE_DECLARATION;
    }

    void addExtendsInterface(const std::string& interfaceName) {
        extendsInterfaces.push_back(interfaceName);
    }

    void addProperty(PropertyNode* property) {
        properties.push_back(property);
        if (property) {
            property->parent = this;
            children.push_back(property);
        }
    }

    void addInterfaceProperty(InterfacePropertyNode* property) {
        interfaceProperties.push_back(property);
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

    void addIndexSignature(ASTNode* indexSignature) {
        indexSignatures.push_back(indexSignature);
        if (indexSignature) {
            indexSignature->parent = this;
            children.push_back(indexSignature);
        }
    }

    void addCallSignature(ASTNode* callSignature) {
        callSignatures.push_back(callSignature);
        if (callSignature) {
            callSignature->parent = this;
            children.push_back(callSignature);
        }
    }

    void addConstructSignature(ASTNode* constructSignature) {
        constructSignatures.push_back(constructSignature);
        if (constructSignature) {
            constructSignature->parent = this;
            children.push_back(constructSignature);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceDeclaration(" << name;
        if (!extendsInterfaces.empty()) {
            os << " extends ";
            for (size_t i = 0; i < extendsInterfaces.size(); ++i) {
                if (i > 0) os << ", ";
                os << extendsInterfaces[i];
            }
        }
        os << ")\n";
        for (auto property : properties) if (property) property->print(os, indent + 1);
        for (auto interfaceProp : interfaceProperties) if (interfaceProp) interfaceProp->print(os, indent + 1);
        for (auto method : methods) if (method) method->print(os, indent + 1);
        for (auto indexSig : indexSignatures) if (indexSig) indexSig->print(os, indent + 1);
        for (auto callSig : callSignatures) if (callSig) callSig->print(os, indent + 1);
        for (auto constructSig : constructSignatures) if (constructSig) constructSig->print(os, indent + 1);
    }
};

class InterfaceIndexSignatureNode : public ASTNode {
public:
    std::string keyName;
    ASTNode* keyType;
    ASTNode* valueType;
    bool isReadonly;

    InterfaceIndexSignatureNode(ASTNode* parent) : ASTNode(parent), keyType(nullptr), valueType(nullptr), isReadonly(false) {
        nodeType = ASTNodeType::INTERFACE_INDEX_SIGNATURE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceIndexSignature";
        if (isReadonly) os << "(readonly";
        os << "[" << keyName << ": ";
        // Print key type
        if (keyType) {
            if (keyType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(keyType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
        }
        os << "]: ";
        // Print value type
        if (valueType) {
            if (valueType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(valueType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
        }
        os << ")\n";
    }
};

class InterfaceCallSignatureNode : public ASTNode {
public:
    ParameterListNode* parameters;
    ASTNode* returnType;

    InterfaceCallSignatureNode(ASTNode* parent) : ASTNode(parent), parameters(nullptr), returnType(nullptr) {
        nodeType = ASTNodeType::INTERFACE_CALL_SIGNATURE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceCallSignature\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) {
            os << pad() << "  ReturnType: ";
            if (returnType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(returnType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
            os << "\n";
        }
    }
};

class InterfaceConstructSignatureNode : public ASTNode {
public:
    ParameterListNode* parameters;
    ASTNode* returnType;

    InterfaceConstructSignatureNode(ASTNode* parent) : ASTNode(parent), parameters(nullptr), returnType(nullptr) {
        nodeType = ASTNodeType::INTERFACE_CONSTRUCT_SIGNATURE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InterfaceConstructSignature\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) {
            os << pad() << "  ReturnType: ";
            if (returnType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(returnType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
            os << "\n";
        }
    }
};

class ClassPropertyNode : public ASTNode {
public:
    std::string name;
    TypeAnnotationNode* typeAnnotation;
    ExpressionNode* initializer;
    AccessModifier accessModifier;
    bool isStatic;
    bool isReadonly;

    ClassPropertyNode(ASTNode* parent) : ASTNode(parent), typeAnnotation(nullptr), initializer(nullptr), accessModifier(AccessModifier::BLOCK), isStatic(false), isReadonly(false) {
        nodeType = ASTNodeType::CLASS_PROPERTY;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassProperty(" << name;
        if (accessModifier != AccessModifier::BLOCK) {
            static const char* accessMap[] = {"public", "private", "protected", ""};
            os << ", " << accessMap[static_cast<int>(accessModifier)];
        }
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
    AccessModifier accessModifier;
    bool isStatic;
    bool isAbstract;

    ClassMethodNode(ASTNode* parent) : ASTNode(parent), parameters(nullptr), returnType(nullptr), body(nullptr), accessModifier(AccessModifier::BLOCK), isStatic(false), isAbstract(false) {
        nodeType = ASTNodeType::CLASS_METHOD;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassMethod(" << name;
        if (accessModifier != AccessModifier::BLOCK) {
            static const char* accessMap[] = {"public", "private", "protected", ""};
            os << ", " << accessMap[static_cast<int>(accessModifier)];
        }
        if (isStatic) os << ", static";
        if (isAbstract) os << ", abstract";
        os << ")\n";
        if (parameters) parameters->print(os, indent + 1);
        if (returnType) returnType->print(os, indent + 1);
        if (body) body->print(os, indent + 1);
    }
};

class ClassDeclarationNode : public LexicalScopeNode {
public:
    std::string name;
    GenericTypeParametersNode* genericParameters;
    std::string extendsClass;
    std::vector<std::string> implementsInterfaces;
    std::vector<ClassPropertyNode*> properties;
    std::vector<ClassMethodNode*> methods;
    bool isAbstract;

    // Compatibility members for codegen
    std::string className; // Alias for name

    // Analysis: Class inheritance and layout information
    std::vector<std::string> parentClassNames;  // Names of parent classes (from parsing)
    std::vector<ClassDeclarationNode*> parentRefs;     // Resolved parent class pointers (from analysis)
    std::map<std::string, VariableInfo> fields; // field name -> VariableInfo with offset, size, etc.
    std::map<std::string, int> parentOffsets;   // parent class name -> offset in object layout
    std::vector<std::string> allFieldsInOrder;  // All fields (parents + own) in layout order
    int totalSize = 0;                 // Total size of all fields (no header, no methods, just data)

    // Method closure layout information (build-time layout descriptor)
    struct MethodLayoutInfo {
        std::string methodName;
        FunctionDeclarationNode* method;  // Pointer to the method's FunctionDeclNode (build-time)
        int thisOffset;            // Offset adjustment needed for this pointer (for inherited methods)
        ClassDeclarationNode* definingClass; // Which class originally defined this method
        int closureSize = 0;       // Size of the closure for this method
        int closureOffsetInObject = 0; // Offset in object where this method's closure is stored
        VariableInfo closurePointerField; // Virtual field for the closure pointer in object layout
    };
    std::vector<MethodLayoutInfo> methodLayout;

    ClassDeclarationNode(ASTNode* parent) : LexicalScopeNode(parent, true), genericParameters(nullptr), isAbstract(false) {
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

    void walk() override {
        std::cout << "walkClassDeclaration" << std::endl;
        for (auto child : children) {
            if (child) child->walk();
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassDeclaration(" << name;
        if (isAbstract) os << ", abstract";
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

class ClassExpressionNode : public ExpressionNode {
public:
    std::string name; // Optional for named class expressions
    std::string extendsClass;
    std::vector<std::string> implementsInterfaces;
    std::vector<ClassPropertyNode*> properties;
    std::vector<ClassMethodNode*> methods;
    bool isAbstract;

    ClassExpressionNode(ASTNode* parent) : ExpressionNode(parent), isAbstract(false) {
        nodeType = ASTNodeType::CLASS_EXPRESSION;
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
        os << pad() << "ClassExpression";
        if (!name.empty()) os << "(" << name << ")";
        if (isAbstract) os << " abstract";
        if (!extendsClass.empty()) os << " extends " << extendsClass;
        if (!implementsInterfaces.empty()) {
            os << " implements ";
            for (size_t i = 0; i < implementsInterfaces.size(); ++i) {
                if (i > 0) os << ", ";
                os << implementsInterfaces[i];
            }
        }
        os << "\n";
        for (auto property : properties) if (property) property->print(os, indent + 1);
        for (auto method : methods) if (method) method->print(os, indent + 1);
    }
};

class ClassGetterNode : public ASTNode {
public:
    std::string name;
    TypeAnnotationNode* returnType;
    ASTNode* body;
    AccessModifier accessModifier;
    bool isStatic;

    ClassGetterNode(ASTNode* parent) : ASTNode(parent), returnType(nullptr), body(nullptr), accessModifier(AccessModifier::BLOCK), isStatic(false) {
        nodeType = ASTNodeType::CLASS_GETTER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassGetter(" << name;
        if (accessModifier != AccessModifier::BLOCK) {
            static const char* accessMap[] = {"public", "private", "protected", ""};
            os << ", " << accessMap[static_cast<int>(accessModifier)];
        }
        if (isStatic) os << ", static";
        os << ")\n";
        if (returnType) returnType->print(os, indent + 1);
        if (body) body->print(os, indent + 1);
    }
};

class ClassSetterNode : public ASTNode {
public:
    std::string name;
    ParameterNode* parameter;
    ASTNode* body;
    AccessModifier accessModifier;
    bool isStatic;

    ClassSetterNode(ASTNode* parent) : ASTNode(parent), parameter(nullptr), body(nullptr), accessModifier(AccessModifier::BLOCK), isStatic(false) {
        nodeType = ASTNodeType::CLASS_SETTER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ClassSetter(" << name;
        if (accessModifier != AccessModifier::BLOCK) {
            static const char* accessMap[] = {"public", "private", "protected", ""};
            os << ", " << accessMap[static_cast<int>(accessModifier)];
        }
        if (isStatic) os << ", static";
        os << ")\n";
        if (parameter) parameter->print(os, indent + 1);
        if (body) body->print(os, indent + 1);
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

class IntersectionTypeNode : public ASTNode {
public:
    std::vector<ASTNode*> types;

    IntersectionTypeNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::INTERSECTION_TYPE;
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
        os << pad() << "IntersectionType\n";
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

class AwaitExpressionNode : public ExpressionNode {
public:
    ExpressionNode* argument;

    AwaitExpressionNode(ASTNode* parent) : ExpressionNode(parent), argument(nullptr) {
        nodeType = ASTNodeType::AWAIT_EXPRESSION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "AwaitExpression\n";
        if (argument) argument->print(os, indent + 1);
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

// Module declarations
class ImportSpecifier : public ASTNode {
public:
    std::string imported;
    std::string local;

    ImportSpecifier(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::IMPORT_SPECIFIER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ImportSpecifier(imported: " << imported << ", local: " << local << ")\n";
    }
};

class ImportDefaultSpecifier : public ASTNode {
public:
    std::string local;

    ImportDefaultSpecifier(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::IMPORT_DEFAULT_SPECIFIER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ImportDefaultSpecifier(" << local << ")\n";
    }
};

class ImportNamespaceSpecifier : public ASTNode {
public:
    std::string local;

    ImportNamespaceSpecifier(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::IMPORT_NAMESPACE_SPECIFIER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ImportNamespaceSpecifier(" << local << ")\n";
    }
};

class ImportDeclaration : public ASTNode {
public:
    std::string source;
    ImportDefaultSpecifier* defaultSpecifier;
    ImportNamespaceSpecifier* namespaceSpecifier;
    std::vector<ImportSpecifier*> specifiers;

    ImportDeclaration(ASTNode* parent) : ASTNode(parent), defaultSpecifier(nullptr), namespaceSpecifier(nullptr) {
        nodeType = ASTNodeType::IMPORT_DECLARATION;
    }

    void setDefaultSpecifier(ImportDefaultSpecifier* specifier) {
        defaultSpecifier = specifier;
        if (specifier) {
            specifier->parent = this;
            children.push_back(specifier);
        }
    }

    void setNamespaceSpecifier(ImportNamespaceSpecifier* specifier) {
        namespaceSpecifier = specifier;
        if (specifier) {
            specifier->parent = this;
            children.push_back(specifier);
        }
    }

    void addSpecifier(ImportSpecifier* specifier) {
        specifiers.push_back(specifier);
        if (specifier) {
            specifier->parent = this;
            children.push_back(specifier);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ImportDeclaration(from: " << source << ")\n";
        if (defaultSpecifier) defaultSpecifier->print(os, indent + 1);
        if (namespaceSpecifier) namespaceSpecifier->print(os, indent + 1);
        for (auto specifier : specifiers) if (specifier) specifier->print(os, indent + 1);
    }
};

class ExportSpecifier : public ASTNode {
public:
    std::string local;
    std::string exported;

    ExportSpecifier(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::EXPORT_SPECIFIER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ExportSpecifier(local: " << local << ", exported: " << exported << ")\n";
    }
};

class ExportNamedDeclaration : public ASTNode {
public:
    std::string source; // For re-exports
    std::vector<ExportSpecifier*> specifiers;

    ExportNamedDeclaration(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::EXPORT_NAMED_DECLARATION;
    }

    void addSpecifier(ExportSpecifier* specifier) {
        specifiers.push_back(specifier);
        if (specifier) {
            specifier->parent = this;
            children.push_back(specifier);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ExportNamedDeclaration";
        if (!source.empty()) os << "(from: \"" << source << "\")";
        os << "\n";
        for (auto specifier : specifiers) if (specifier) specifier->print(os, indent + 1);
    }
};

class ExportDefaultDeclaration : public ASTNode {
public:
    ASTNode* declaration;

    ExportDefaultDeclaration(ASTNode* parent) : ASTNode(parent), declaration(nullptr) {
        nodeType = ASTNodeType::EXPORT_DEFAULT_DECLARATION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ExportDefaultDeclaration\n";
        if (declaration) declaration->print(os, indent + 1);
    }
};

class ExportAllDeclaration : public ASTNode {
public:
    std::string source;
    std::string exported; // For "export * as name from 'module'"

    ExportAllDeclaration(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::EXPORT_ALL_DECLARATION;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ExportAllDeclaration(from: \"" << source << "\"";
        if (!exported.empty()) os << ", as: " << exported;
        os << ")\n";
    }
};

// Destructuring nodes
class ArrayDestructuringNode : public ASTNode {
public:
    std::vector<std::string> elements;
    std::string restElement;

    ArrayDestructuringNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::ARRAY_DESTRUCTURING;
    }

    void addElement(const std::string& element) {
        elements.push_back(element);
    }

    void setRestElement(const std::string& rest) {
        restElement = rest;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ArrayDestructuring[";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (i > 0) os << ", ";
            os << elements[i];
        }
        if (!restElement.empty()) {
            if (!elements.empty()) os << ", ";
            os << "..." << restElement;
        }
        os << "]\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

class ObjectDestructuringNode : public ASTNode {
public:
    std::vector<std::pair<std::string, std::string>> properties;
    std::string restElement;

    ObjectDestructuringNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::OBJECT_DESTRUCTURING;
    }

    void addProperty(const std::string& key, const std::string& value) {
        properties.emplace_back(key, value);
    }

    void setRestElement(const std::string& rest) {
        restElement = rest;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ObjectDestructuring{";
        for (size_t i = 0; i < properties.size(); ++i) {
            if (i > 0) os << ", ";
            const auto& prop = properties[i];
            if (prop.first == prop.second) {
                os << prop.first;
            } else {
                os << prop.first << ": " << prop.second;
            }
        }
        if (!restElement.empty()) {
            if (!properties.empty()) os << ", ";
            os << "..." << restElement;
        }
        os << "}\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

// Enum nodes
class EnumMemberNode : public ASTNode {
public:
    std::string name;
    ExpressionNode* initializer;

    EnumMemberNode(ASTNode* parent) : ASTNode(parent), initializer(nullptr) {
        nodeType = ASTNodeType::ENUM_MEMBER;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "EnumMember(" << name;
        if (initializer) {
            os << " = ";
            // For now, just indicate there's an initializer
            os << "value";
        }
        os << ")\n";
        if (initializer) initializer->print(os, indent + 1);
    }
};

class TypeAliasNode : public ASTNode {
public:
    std::string name;
    GenericTypeParametersNode* genericParameters;
    ASTNode* typeAnnotation;

    TypeAliasNode(ASTNode* parent) : ASTNode(parent), genericParameters(nullptr), typeAnnotation(nullptr) {
        nodeType = ASTNodeType::TYPE_ALIAS;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "TypeAlias(" << name << ")\n";
        if (genericParameters) genericParameters->print(os, indent + 1);
        if (typeAnnotation) {
            os << pad() << "  Type:\n";
            typeAnnotation->print(os, indent + 2);
        }
    }
};

class EnumDeclarationNode : public ASTNode {
public:
    std::string name;
    std::vector<EnumMemberNode*> members;
    bool isConst;

    EnumDeclarationNode(ASTNode* parent) : ASTNode(parent), isConst(false) {
        nodeType = ASTNodeType::ENUM_DECLARATION;
    }

    void addMember(EnumMemberNode* member) {
        members.push_back(member);
        if (member) {
            member->parent = this;
            children.push_back(member);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "EnumDeclaration(" << name;
        if (isConst) os << ", const";
        os << ")\n";
        for (auto member : members) if (member) member->print(os, indent + 1);
    }
};

// Advanced generics nodes
class ConditionalTypeNode : public ASTNode {
public:
    ASTNode* checkType;
    ASTNode* extendsType;
    ASTNode* trueType;
    ASTNode* falseType;

    ConditionalTypeNode(ASTNode* parent) : ASTNode(parent), checkType(nullptr), extendsType(nullptr), trueType(nullptr), falseType(nullptr) {
        nodeType = ASTNodeType::CONDITIONAL_TYPE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ConditionalType\n";
        if (checkType) {
            os << pad() << "  Check:\n";
            checkType->print(os, indent + 2);
        }
        if (extendsType) {
            os << pad() << "  Extends:\n";
            extendsType->print(os, indent + 2);
        }
        if (trueType) {
            os << pad() << "  True:\n";
            trueType->print(os, indent + 2);
        }
        if (falseType) {
            os << pad() << "  False:\n";
            falseType->print(os, indent + 2);
        }
    }
};

class InferTypeNode : public ASTNode {
public:
    std::string typeParameter;

    InferTypeNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::INFER_TYPE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "InferType(" << typeParameter << ")\n";
    }
};

class TemplateLiteralTypeNode : public ASTNode {
public:
    std::vector<std::string> quasis;
    std::vector<ASTNode*> types;

    TemplateLiteralTypeNode(ASTNode* parent) : ASTNode(parent) {
        nodeType = ASTNodeType::TEMPLATE_LITERAL_TYPE;
    }

    void addQuasi(const std::string& quasi) {
        quasis.push_back(quasi);
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
        os << pad() << "TemplateLiteralType\n";
        for (size_t i = 0; i < quasis.size(); ++i) {
            os << pad() << "  Quasi: \"" << quasis[i] << "\"\n";
            if (i < types.size() && types[i]) {
                os << pad() << "  Type:\n";
                types[i]->print(os, indent + 2);
            }
        }
    }
};

class MappedTypeNode : public ASTNode {
public:
    std::string typeParameter;
    ASTNode* constraintType;
    ASTNode* valueType;
    std::string asClause; // For key remapping
    bool isReadonly;
    bool isOptional;

    MappedTypeNode(ASTNode* parent) : ASTNode(parent), constraintType(nullptr), valueType(nullptr), isReadonly(false), isOptional(false) {
        nodeType = ASTNodeType::MAPPED_TYPE;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "MappedType";
        if (isReadonly) os << "(readonly";
        os << "[" << typeParameter << " in ";
        // Print constraint type
        if (constraintType) {
            if (constraintType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(constraintType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
        }
        if (!asClause.empty()) {
            os << " as " << asClause;
        }
        os << "]";
        if (isOptional) os << "?";
        os << ": ";
        // Print value type
        if (valueType) {
            if (valueType->nodeType == ASTNodeType::TYPE_ANNOTATION) {
                auto* typeNode = static_cast<TypeAnnotationNode*>(valueType);
                static const char* typeMap[] = {"int64","float64","string","raw_memory","object"};
                os << typeMap[static_cast<int>(typeNode->dataType)];
            } else {
                os << "type";
            }
        }
        os << ")\n";
        for (auto child : children) if (child) child->print(os, indent + 1);
    }
};

// Member access node - for accessing object properties (obj.field)
class MemberAccessNode : public ExpressionNode {
public:
    ExpressionNode* object;  // The object being accessed
    std::string memberName;
    ClassDeclarationNode* classRef = nullptr; // Analysis: resolved class reference
    int memberOffset = 0; // Analysis: byte offset of the field in the object

    MemberAccessNode(ASTNode* parent, const std::string& member)
        : ExpressionNode(parent), object(nullptr), memberName(member) {
        nodeType = ASTNodeType::MEMBER_ACCESS;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "MemberAccess(" << memberName << ")\n";
        if (object) object->print(os, indent + 1);
    }
};

// Member assignment node - for assigning to object properties (obj.field = value)
class MemberAssignNode : public ASTNode {
public:
    MemberAccessNode* member;
    ExpressionNode* value;

    MemberAssignNode(ASTNode* parent) : ASTNode(parent), member(nullptr), value(nullptr) {
        nodeType = ASTNodeType::MEMBER_ASSIGN;
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "MemberAssign\n";
        if (member) member->print(os, indent + 1);
        if (value) value->print(os, indent + 1);
    }
};

// Method call node - for calling methods on objects (obj.method(args))
class MethodCallNode : public ExpressionNode {
public:
    ExpressionNode* object;            // The object expression
    std::string methodName;            // Name of the method to call
    std::vector<ExpressionNode*> args; // Method arguments

    // Analysis: resolved information
    FunctionDeclarationNode* resolvedMethod = nullptr; // The actual method function
    int methodLayoutIndex = -1;        // Index in the method layout
    int thisOffset = 0;                // Offset to adjust this pointer
    ClassDeclarationNode* objectClass = nullptr;      // Class of the object
    int methodClosureOffset = 0;       // Offset in object where method closure is stored

    // Compatibility members for codegen
    VariableInfo* varRef = nullptr;

    MethodCallNode(ASTNode* parent, const std::string& method)
        : ExpressionNode(parent), object(nullptr), methodName(method) {
        nodeType = ASTNodeType::METHOD_CALL;
    }

    void addArg(ExpressionNode* arg) {
        args.push_back(arg);
        if (arg) {
            arg->parent = this;
            children.push_back(arg);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "MethodCall(" << methodName << ")\n";
        if (object) object->print(os, indent + 1);
        for (auto arg : args) if (arg) arg->print(os, indent + 1);
    }
};

// This expression - represents 'this' keyword in methods
class ThisExprNode : public ExpressionNode {
public:
    ClassDeclarationNode* classContext = nullptr;  // Analysis: which class this method belongs to
    FunctionDeclarationNode* methodContext = nullptr; // Analysis: the method this 'this' appears in

    ThisExprNode(ASTNode* parent) : ExpressionNode(parent) {
        nodeType = ASTNodeType::THIS_EXPR;
        value = "this";
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "ThisExpr\n";
    }
};

// New expression - for creating objects (new ClassName(args))
class NewExprNode : public ExpressionNode {
public:
    std::string className;
    ClassDeclarationNode* classRef = nullptr; // Analysis: resolved class reference
    std::vector<ExpressionNode*> args;
    bool isRawMemory = false;

    NewExprNode(ASTNode* parent, const std::string& name)
        : ExpressionNode(parent), className(name), classRef(nullptr) {
        nodeType = ASTNodeType::NEW_EXPR;
    }

    void addArg(ExpressionNode* arg) {
        args.push_back(arg);
        if (arg) {
            arg->parent = this;
            children.push_back(arg);
        }
    }

    void print(std::ostream& os, int indent) const override {
        auto pad = [indent]() { return string(indent * 2, ' '); };
        os << pad() << "NewExpr(" << className << ")\n";
        for (auto arg : args) if (arg) arg->print(os, indent + 1);
    }
};
