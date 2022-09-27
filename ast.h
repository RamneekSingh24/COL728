#include <vector>
#include <unordered_map>

#ifndef AST_H
#define AST_H

namespace ast {

    //template <typename T>
    class yyAST {
    public:
        void eval();

        virtual std::string name() {
            return "yyAST";
        }

        virtual ~yyAST() {}

        std::vector<yyAST *> nodes;

        void addNode(yyAST *node) {
            nodes.push_back(node);
        }

        virtual void print(int indent = 0) {
            std::cout << std::string(2 * indent, ' ') << name() << "---\n";
            for (auto node : nodes) {
                node->print(indent + 1);
            }
            std::cout << std::string(2 * indent, ' ') << name() << "---\n";
        }
    };


    class yyTU : public yyAST {
    public:
        std::string name() {
            return "yyTU";
        }

        void addDecl(yyAST *decl) {
            nodes.push_back(decl);
        }

    };


    class yyDeclaration : public yyAST {
        // TODO
        std::string name() {
            return "yyDeclaration";
        }
    };


    enum yySimpleType {
        TYPE_INT,
        TYPE_VOID,
        TYPE_FLOAT
    };


    class yyTypeSpecifier : public yyAST {

    public:
        std::string name() {
            return "yyTypeSpecifier";
        }

        std::string typeName() {
            switch (type) {
                case yySimpleType::TYPE_INT:
                    return "int";
                case yySimpleType::TYPE_FLOAT:
                    return "float";
                case yySimpleType::TYPE_VOID:
                    return "void";
            }
        }

        yySimpleType type;

        yyTypeSpecifier(yySimpleType typeSpec) {
            type = typeSpec;
        }

        void print(int indent = 0) {
            std::cout << std::string(2 * indent, ' ') << "Type: " << typeName() << "\n";
        }
    };


    class yyDeclSpecifiers : public yyAST {
    public:
        std::string name() {
            return "yyDeclSpecifiers";
        }

        yyDeclSpecifiers(yyAST *specifier) {
            yyTypeSpecifier *type = dynamic_cast<yyTypeSpecifier *> (specifier);
            assert(type != nullptr || puts("non-type decl specifier not supported yet..") && 0);
            nodes.push_back(type);
        }
    };


    class yyIdentifier : public yyAST {
    public:
        std::string name() {
            return "yyIdentifier";
        }

        std::string id;

        yyIdentifier(char *idTok) {
            id = std::string(idTok);
        }

        void print(int indent = 0) {
            std::cout << std::string(2 * indent, ' ') << "ID: " << id << "\n";
        }
    };

    class yyIntegerLiteral : public yyAST {
    public:
        std::string name() {
            return "yyIntegerLiteral";
        }

        int v;

        yyIntegerLiteral(int val) {
            v = val;
        }

        void print(int indent = 0) {
            std::cout << std::string(2 * indent, ' ') << "INT: " << v << "\n";
        }
    };

    class yyFloatLiteral : public yyAST {
    public:
        std::string name() {
            return "yyFloatLiteral";
        }

        float v;

        yyFloatLiteral(float val) {
            v = val;
        }

        void print(int indent = 0) {
            std::cout << std::string(2 * indent, ' ') << "FLOAT: " << v << "\n";
        }
    };


    class yyPointer : public yyAST {/* TODO */};

    class yyDirectDeclarator : public yyAST {
    public:
        std::string name() {
            return "yyDirectDeclarator";
        }

        yyDirectDeclarator(yyAST *identifier) {
            assert(dynamic_cast<yyIdentifier *> (identifier) != nullptr);
            nodes.push_back(identifier);
        }

        yyDirectDeclarator(yyAST *directDeclarator, yyAST *parameterList) {
            assert(directDeclarator->nodes.size() == 1 &&
                   dynamic_cast<yyIdentifier *> (directDeclarator->nodes[0]) != nullptr);
            nodes.push_back(directDeclarator->nodes[0]);
            nodes.push_back(parameterList);
        }
    };


    class yyDeclarator : public yyAST {
    public:
        std::string name() {
            return "yyDeclarator";
        }

        std::vector<yyAST *> pointers;
        yyAST *directDeclarator;

        yyDeclarator(yyAST *directDeclarator_) {
            nodes.push_back(directDeclarator_);
            directDeclarator = directDeclarator_;
        }
    };

    class yyParameterDecl : public yyAST {
    public:
        std::string name() {
            return "yyParameterDecl";
        }

        yyParameterDecl(yyAST *declSpecifiers, yyAST *decl) {
            nodes.push_back(declSpecifiers);
            nodes.push_back(decl);
        }
    };


    class yyParameterList : public yyAST {
    public:
        std::string name() {
            return "yyParameterList";
        }

        yyParameterList() = default;

        yyParameterList(yyAST *param) {
            nodes.push_back(param);
        }

        void addParam(yyAST *param) {
            nodes.push_back(param);
        }
    };


    class yyFunctionDefinition : public yyAST {
    public:
        std::string name() {
            return "yyFunctionDefinition";
        }

        yyFunctionDefinition(yyAST *declSpecs, yyAST *declarator, yyAST *code) {
            nodes.push_back(declSpecs);
            nodes.push_back(declarator);
            nodes.push_back(code);
        }
    };

    class yyCompoundStatement : public yyAST {
    public:
        std::string name() {
            return "yyCompoundStatement";
        }

        // decls or statements
        yyCompoundStatement() {};

        yyCompoundStatement(yyAST *blockItem) {
            // blockItem = decl or statement
            nodes.push_back(blockItem);
        }

        void addItem(yyAST *blockItem) {
            // blockItem = decl or statement
            nodes.push_back(blockItem);
        }
    };

    class yyStatement : public yyAST {
    public:
        std::string name() {
            return "yyStatement";
        }
    };

    class yyAssignmentExpression : public yyAST {
    public:
        std::string name() {
            return "yyAssignmentExpression";
        }
    };

    class yyConditionalExpression : public yyAST {
    public:
        std::string name() {
            return "yyConditionalExpression";
        }
    };


    class yyExpression : public yyAST {
    public:
        std::string name() {
            return "yyExpression";
        }

        yyExpression() {}; // emtpy expression
        yyExpression(yyAST *assignmentExpression) {
            nodes.push_back(assignmentExpression);
        }

        void addAssignmentExpression(yyAST *assignmentExpression) {
            nodes.push_back(assignmentExpression);
        }
    };

    enum BinaryOp {
        PLUS,
        MINUS,
        MULT,
        DIV,
        MOD,
        OR,
        AND,
        XOR,
        LSHIFT,
        RSHIFT,
        GT,
        GTE,
        LT,
        LTE,
        EQUAL,
        NOT_EQUAL,
        LOGICAL_OR,
        LOGICAL_AND,
        FUNC_CALL
    };


    class yyBinaryOp : public yyAST {
    public:
        BinaryOp binaryOp;

        std::string name() {
            switch (binaryOp) {
                case BinaryOp::PLUS:
                    return "PLUS";
                case BinaryOp::MINUS:
                    return "MINUS";
                case BinaryOp::MULT:
                    return "MULT";
                case BinaryOp::DIV:
                    return "DIV";
                case BinaryOp::MOD:
                    return "MOD";
                case BinaryOp::OR:
                    return "OR";
                case BinaryOp::AND:
                    return "AND";
                case BinaryOp::XOR:
                    return "XOR";
                case BinaryOp::LSHIFT:
                    return "LSHIFT";
                case BinaryOp::RSHIFT:
                    return "RSHIFT";
                case BinaryOp::GT:
                    return "GREATER THAN";
                case BinaryOp::GTE:
                    return "GREATER THAN EQUAL TO";
                case BinaryOp::LT:
                    return "LESS THAN";
                case BinaryOp::LTE:
                    return "LESS THAN EQUAL TO";
                case BinaryOp::EQUAL:
                    return "EQUAL?";
                case BinaryOp::NOT_EQUAL:
                    return "NOT EQUAL?";
                case BinaryOp::LOGICAL_OR:
                    return "LOGICAL OR";
                case BinaryOp::LOGICAL_AND:
                    return "LOGICAL AND";
                case BinaryOp::FUNC_CALL:
                    return "FUNCTION CALL";
            }
        }

        yyBinaryOp(yyAST *left, BinaryOp binOp, yyAST *right) {
            nodes.push_back(left);
            nodes.push_back(right);
            binaryOp = binOp;
        }
    };

    enum UnaryOp {
        PL,
        NEG,
        PRE_INC,
        PRE_DEC,
        POST_INC,
        POST_DEC,
        NOT,
        LOGICAL_NOT
    };

    class yyUnaryOp : public yyAST {
    public:
        UnaryOp unaryOp;

        std::string name() {
            switch (unaryOp) {
                case UnaryOp::PL:
                    return "POSITIVE";
                case UnaryOp::NEG:
                    return "NEGATIVE";
                case UnaryOp::PRE_INC:
                    return "PRE INCREMENT";
                case UnaryOp::PRE_DEC:
                    return "PRE DECREMENT";
                case UnaryOp::POST_INC:
                    return "POST INCREMENT";
                case UnaryOp::POST_DEC:
                    return "POST DECREMENT";
                case UnaryOp::NOT:
                    return "NOT";
                case UnaryOp::LOGICAL_NOT:
                    return "LOGICAL NOT";
            }
        }

        yyUnaryOp(UnaryOp op, yyAST *opr) {
            unaryOp = op;
            nodes.push_back(opr);
        }
    };


    class yyArgumentExpressionList : public yyAST {
    public:
        yyArgumentExpressionList() {}; // empty list
    };
}
#endif
