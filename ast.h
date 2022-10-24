#include <vector>
#include <iostream>
#include <unordered_map>
#include <set>
#include "SymbolTable.h"

#ifndef AST_H
#define AST_H

extern int yylineno;

namespace ast
{

    enum yySimpleType
    {
        TYPE_INT,
        TYPE_VOID,
        TYPE_FLOAT,
        TYPE_CHAR,
        TYPE_BOOL,
        TYPE_ELLIPSIS
    };

    class Type
    {
    public:
        virtual std::string typeStr()
        {
            return "no-type";
        }
        virtual bool equals(Type *other)
        {
            return this->typeStr() == other->typeStr();
        }
    };

    class SimpleType : public Type
    {

    public:
        yySimpleType simpleType;

        SimpleType(yySimpleType simpleType) : simpleType(simpleType){};

        std::string typeStr()
        {
            switch (simpleType)
            {
            case TYPE_INT:
                return "int";
            case TYPE_VOID:
                return "void";
            case TYPE_FLOAT:
                return "float";
            case TYPE_CHAR:
                return "char";
            case TYPE_BOOL:
                return "bool";
            case TYPE_ELLIPSIS:
                return "...";
            }
            return "error";
        }
    };

    class PointerType : public Type
    {
        int pointer_cnt = 0;
        SimpleType simpleType;

    public:
        PointerType(int pointer_cnt, SimpleType simpleType) : pointer_cnt(pointer_cnt), simpleType(simpleType){};

        PointerType addPointer()
        {
            PointerType newType(this->pointer_cnt + 1, this->simpleType);
            return newType;
        }
        PointerType removePointer()
        {
            PointerType newType(this->pointer_cnt - 1, this->simpleType);
            return newType;
        }

        std::string typeStr()
        {
            std::string typeStr = "";
            for (int i = 0; i < pointer_cnt; i++)
            {
                typeStr += "*";
            }
            typeStr += simpleType.typeStr();
            return typeStr;
        }
    };

    class FunctionType : public Type
    {
    public:
        std::vector<Type *> paramTypes;
        Type *returnType;
        FunctionType(std::vector<Type *> paramTypes, Type *returnType) : paramTypes(paramTypes), returnType(returnType){};

        std::string typeStr()
        {
            std::string typeStr = "";
            typeStr += returnType->typeStr();
            typeStr += "(";
            for (int i = 0; i < paramTypes.size(); i++)
            {
                typeStr += paramTypes[i]->typeStr();
                if (i != paramTypes.size() - 1)
                {
                    typeStr += ",";
                }
            }
            typeStr += ")";
            return typeStr;
        }
    };

    // template <typename T>
    class yyAST
    {
    public:
        int line_no;

        bool dont_drop_env = false;

        Type *my_type = new SimpleType(TYPE_VOID);

        yyAST() : line_no(yylineno){};

        void eval();

        virtual std::string name()
        {
            return "yyAST";
        }

        virtual ~yyAST() {}

        std::vector<yyAST *> nodes;

        void addNode(yyAST *node)
        {
            nodes.push_back(node);
        }

        virtual void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << name() << "---\n";
            for (auto node : nodes)
            {
                node->print(indent + 1);
            }
            std::cout << std::string(2 * indent, ' ') << name() << "---\n";
        }

        virtual bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool result = true;
            for (auto node : nodes)
            {
                result &= node->envCheck(symTable);
            }
            return result;
        }

        virtual bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool result = true;
            for (auto node : nodes)
            {
                result &= node->typeCheck(symTable);
            }
            my_type = new SimpleType(TYPE_VOID);
            return result;
        }
    };

    class yyTU : public yyAST
    {
    public:
        std::string name()
        {
            return "yyTU";
        }

        void addDecl(yyAST *decl)
        {
            nodes.push_back(decl);
        }

        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool result = true;
            symTable->createNewEnv();
            for (auto node : nodes)
            {
                result &= node->envCheck(symTable);
            }
            return result;
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            symTable->createNewEnv();
            bool result = true;
            for (auto node : nodes)
            {
                result &= node->typeCheck(symTable);
            }
            my_type = new SimpleType(TYPE_VOID);
            return result;
        }
    };

    class yyTypeSpecifier : public yyAST
    {

    public:
        std::string name()
        {
            return "yyTypeSpecifier";
        }

        std::string typeName()
        {
            switch (type)
            {
            case yySimpleType::TYPE_INT:
                return "int";
            case yySimpleType::TYPE_FLOAT:
                return "float";
            case yySimpleType::TYPE_VOID:
                return "void";
            case yySimpleType::TYPE_CHAR:
                return "char";
            default:
                return "error";
            }
        }

        yySimpleType type;

        yyTypeSpecifier(yySimpleType typeSpec)
        {
            type = typeSpec;
        }

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "Type: " << typeName() << "\n";
        }
        // Base implementation of envCheck will work.
    };

    enum TypeQualifier
    {
        CONST
    };

    class yyTypeQualifier : public yyAST
    {
    public:
        std::string name()
        {
            return "yyTypeQualifier";
        }

        TypeQualifier typeQualifier;

        std::string typeQualifierName()
        {
            switch (typeQualifier)
            {
            case CONST:
                return "const";
            default:
                return "error";
            }
        }

        yyTypeQualifier(TypeQualifier typeQualifier) : typeQualifier(typeQualifier){};

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "TypeQualifier: " << typeQualifierName() << "\n";
        }
        // Base implementation of envCheck will work.
    };

    class yyDeclSpecifiers : public yyAST
    {
    public:
        std::string name()
        {
            return "yyDeclSpecifiers";
        }
        yyDeclSpecifiers(){};

        yyDeclSpecifiers(yyAST *specifier)
        {
            // yyTypeSpecifier *type = dynamic_cast<yyTypeSpecifier *>(specifier);
            // assert(type != nullptr || puts("non-type decl specifier not supported yet..") && 0);
            nodes.push_back(specifier);
        }
        yyTypeSpecifier *getType()
        {
            for (auto node : nodes)
            {
                yyTypeSpecifier *type = dynamic_cast<yyTypeSpecifier *>(node);
                if (type != nullptr)
                {
                    return type;
                }
            }

            return nullptr;
        }
        // Base implementation of envCheck will work.
    };

    class yyIdentifier : public yyAST
    {
    public:
        std::string name()
        {
            return "yyIdentifier";
        }

        std::string id;

        yyIdentifier(char *idTok)
        {
            this->id = std::string(idTok);
        }

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "ID: " << id << "\n";
        }

        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = symTable->getFromEnv(id) != nullptr;
            if (!ret)
            {
                std::cout << "[Line No " << line_no << "] Error: " << id << " not declared in this scope\n";
            }
            return ret;
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            yyAST *decl = symTable->getFromEnv(id);
            assert(decl != nullptr);
            my_type = decl->my_type;
            return true;
        }
    };

    class yyIntegerLiteral : public yyAST
    {
    public:
        std::string name()
        {
            return "yyIntegerLiteral";
        }

        int v;

        yyIntegerLiteral(int val)
        {
            v = val;
        }

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "INT: " << v << "\n";
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            return true;
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            my_type = new SimpleType(TYPE_INT);
            return true;
        }
    };

    class yyFloatLiteral : public yyAST
    {
    public:
        std::string name()
        {
            return "yyFloatLiteral";
        }

        float v;

        yyFloatLiteral(float val)
        {
            v = val;
        }

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "FLOAT: " << v << "\n";
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            return true;
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            my_type = new SimpleType(TYPE_FLOAT);
            return true;
        }
    };

    class yyStringLiteral : public yyAST
    {
    public:
        std::string name()
        {
            return "yyStringLiteral";
        }

        std::string v;
        yyStringLiteral(char *val)
        {
            this->v = std::string(val);
        }

        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "STRING: " << v << "\n";
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            return true;
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            my_type = new PointerType(1, SimpleType(TYPE_CHAR));
            return true;
        }
    };

    class yyPointer : public yyAST
    {
    public:
        std::string name()
        {
            return "yyPointer";
        }
        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "Type Spec: Pointer\n";
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            return true;
        }
    };

    class yyDirectDeclarator : public yyAST
    {
    public:
        std::string name()
        {
            return "yyDirectDeclarator";
        }

        yyDirectDeclarator(yyAST *identifier)
        {
            assert(dynamic_cast<yyIdentifier *>(identifier) != nullptr);
            nodes.push_back(identifier);
        }

        yyDirectDeclarator(yyAST *directDeclarator, yyAST *parameterList)
        {
            assert(directDeclarator->nodes.size() == 1 &&
                   dynamic_cast<yyIdentifier *>(directDeclarator->nodes[0]) != nullptr);
            nodes.push_back(directDeclarator->nodes[0]);
            nodes.push_back(parameterList);
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() > 0);
            yyIdentifier *idNode = dynamic_cast<yyIdentifier *>(nodes[0]);
            assert(idNode != nullptr);
            std::string id = idNode->id;

            // std::cout << id << " " << line_no << " " << symTable->table.size() << "\n";

            if (!symTable->addToEnv(id, this))
            {
                std::cerr << "[Line No " << this->line_no << "] Error: '" << id << "' already declared in this scope "
                          << "previous declaration was at line no: " << symTable->getFromEnv(id)->line_no << std::endl;
                ret = false;
            }

            // direct decl. for function

            symTable->createNewEnv();

            for (int i = 1; i < nodes.size(); i++)
            {
                ret &= nodes[i]->envCheck(symTable);
            }

            if (!dont_drop_env)
            {
                symTable->popEnv();
            }

            return ret;
        }

        bool typeCheck(Type *t)
        {
            assert(false); // invariant: should never reach here
            return true;
        }
    };

    class yyDeclarator : public yyAST
    {
    public:
        std::string name()
        {
            return "yyDeclarator";
        }

        std::vector<yyAST *> pointers;
        yyAST *directDeclarator;

        yyDeclarator(yyAST *ptrs, yyAST *directDeclarator_)
        {
            for (auto ptr : ptrs->nodes)
            {
                pointers.push_back(ptr);
                nodes.push_back(ptr);
            }
            nodes.push_back(directDeclarator_);
            directDeclarator = directDeclarator_;
        }

        yyDeclarator(yyAST *directDeclarator_)
        {
            nodes.push_back(directDeclarator_);
            directDeclarator = directDeclarator_;
        }
        // default impl. of envCheck will work.
    };

    class yyParameterDecl : public yyAST
    {
    public:
        std::string name()
        {
            return "yyParameterDecl";
        }

        yyParameterDecl(yyAST *declSpecifiers, yyAST *decl)
        {
            nodes.push_back(declSpecifiers);
            nodes.push_back(decl);
        }
        // default impl. of envCheck will work.
        bool typeCheck(Type *t)
        {
            assert(false); // invariant: should never reach here
            return true;
        }
    };

    class yyEllipsis : public yyAST
    {
    public:
        std::string name()
        {
            return "yyEllipsis";
        }
        void print(int indent = 0)
        {
            std::cout << std::string(2 * indent, ' ') << "Parameter: Ellipsis\n";
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            assert(false); // invariant: should never reach here
            return true;
        }
    };

    class yyParameterList : public yyAST
    {
    public:
        std::string name()
        {
            return "yyParameterList";
        }

        yyParameterList() = default;

        yyParameterList(yyAST *param)
        {
            nodes.push_back(param);
        }

        void addParam(yyAST *param)
        {
            nodes.push_back(param);
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            for (int i = 0; i < nodes.size(); i++)
            {
                yyEllipsis *ellipsis = dynamic_cast<yyEllipsis *>(nodes[i]);
                if (ellipsis != nullptr && i != nodes.size() - 1)
                {
                    // ellipsis should be the last parameter
                    // actually, this is checked by the lexer itself, so this wasn't needed.
                    std::cerr << "[Line No " << this->line_no << "] Error: Ellipsis must be the last parameter in the list"
                              << std::endl;
                    ret = false;
                }
                ret &= nodes[i]->envCheck(symTable);
            }
            return ret;
        }
        bool typeCheck(Type *t)
        {
            assert(false); // invariant: should never reach here
            return true;
        }
    };

    class yyDeclaration : public yyAST
    {
        std::string name()
        {
            return "yyDeclaration";
        }

    public:
        yyDeclaration(yyAST *declSpecs, yyAST *initDeclList)
        {
            nodes.push_back(declSpecs);
            for (auto decl : initDeclList->nodes)
            {
                nodes.push_back(decl);
            }
        }
        // Base implementation of envCheck will work.

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            assert(nodes.size() == 2);
            yyDeclSpecifiers *declSpecs = dynamic_cast<yyDeclSpecifiers *>(nodes[0]);
            assert(declSpecs != nullptr);
            yyTypeSpecifier *typeSpec = declSpecs->getType();
            assert(typeSpec != nullptr);

            Type *type = new SimpleType(typeSpec->type);

            yyDeclarator *decl = dynamic_cast<yyDeclarator *>(nodes[1]);
            assert(decl != nullptr);

            if (decl->pointers.size() > 0)
            {
                type = new PointerType(decl->pointers.size(), SimpleType(typeSpec->type));
            }

            yyDirectDeclarator *directDecl = dynamic_cast<yyDirectDeclarator *>(decl->directDeclarator);
            assert(directDecl != nullptr);
            assert(directDecl->nodes.size() > 0 && directDecl->nodes.size() <= 2);
            yyIdentifier *idNode = dynamic_cast<yyIdentifier *>(directDecl->nodes[0]);
            assert(idNode != nullptr);
            std::string id = idNode->id;

            if (directDecl->nodes.size() == 1)
            {
                // simple declarator
                idNode->my_type = type;
            }
            else
            {
                // function declarator
                yyParameterList *params = dynamic_cast<yyParameterList *>(directDecl->nodes[1]);
                assert(params != nullptr);
                std::vector<Type *> paramTypes;
                for (auto param : params->nodes)
                {
                    yyParameterDecl *paramDecl = dynamic_cast<yyParameterDecl *>(param);
                    yyEllipsis *ellipsis = dynamic_cast<yyEllipsis *>(param);
                    assert(paramDecl != nullptr || ellipsis != nullptr);

                    if (ellipsis != nullptr)
                    {
                        paramTypes.push_back(new SimpleType(TYPE_ELLIPSIS));
                    }
                    else
                    {
                        assert(paramDecl->nodes.size() == 2);
                        yyDeclSpecifiers *paramDeclSpecs = dynamic_cast<yyDeclSpecifiers *>(paramDecl->nodes[0]);
                        assert(paramDeclSpecs != nullptr);
                        yyTypeSpecifier *paramTypeSpec = paramDeclSpecs->getType();
                        assert(paramTypeSpec != nullptr);
                        Type *paramType = new SimpleType(paramTypeSpec->type);
                        yyDeclarator *paramDeclr = dynamic_cast<yyDeclarator *>(paramDecl->nodes[1]);
                        assert(paramDeclr != nullptr);
                        if (paramDeclr->pointers.size() > 0)
                        {
                            paramType = new PointerType(paramDeclr->pointers.size(), SimpleType(paramTypeSpec->type));
                        }
                        paramTypes.push_back(paramType);
                    }
                }

                idNode->my_type = new FunctionType(paramTypes, type);
            }

            if (symTable->addToEnv(id, idNode))
            {
                return true;
            }
            else
            {
                // This should not happen though, as this is checked in envCheck.
                std::cerr << "[Line No " << this->line_no << "] Error: '" << id << "' already declared in this scope "
                          << "previous declaration was at line no: " << symTable->getFromEnv(id)->line_no << std::endl;
                return false;
            }
        }
    };

    class yyExpression : public yyAST
    {
    public:
        std::string name()
        {
            return "yyExpression";
        }

        yyExpression(){}; // emtpy expression
        yyExpression(yyAST *assignmentExpression)
        {
            nodes.push_back(assignmentExpression);
        }

        void addAssignmentExpression(yyAST *assignmentExpression)
        {
            nodes.push_back(assignmentExpression);
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            if (nodes.size() == 0)
            {
                this->my_type = new SimpleType(TYPE_VOID);
                return true;
            }
            for (auto node : nodes)
            {
                ret &= node->typeCheck(symTable);
            }
            this->my_type = nodes[nodes.size() - 1]->my_type;
            return ret;
        }
    };

    static Type *merge_statement_types(Type *stat1_type, Type *stat2_type)
    {
        SimpleType *stat1_simple_type = dynamic_cast<SimpleType *>(stat1_type);
        SimpleType *stat2_simple_type = dynamic_cast<SimpleType *>(stat2_type);
        if (stat1_simple_type != nullptr && stat1_simple_type->simpleType == TYPE_VOID)
        {
            return stat2_type;
        }
        else if (stat2_simple_type != nullptr && stat2_simple_type->simpleType == TYPE_VOID)
        {
            return stat1_type;
        }
        else if (stat1_type->equals(stat2_type))
        {
            return stat1_type;
        }
        else
        {
            return nullptr;
        }
    }

    class yyCompoundStatement : public yyAST
    {
    public:
        bool dont_create_new_env = false;
        std::string name()
        {
            return "yyCompoundStatement";
        }

        // decls or statements
        yyCompoundStatement(){};

        yyCompoundStatement(yyAST *blockItem)
        {
            // blockItem = decl or statement
            nodes.push_back(blockItem);
        }

        void addBlockItem(yyAST *blockItem)
        {
            // blockItem = decl or statement
            nodes.push_back(blockItem);
        }
        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            symTable->createNewEnv();
            bool ret = true;
            for (auto node : nodes)
            {
                ret &= node->envCheck(symTable);
            }
            symTable->popEnv();
            return ret;
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            if (!dont_create_new_env)
            {
                symTable->createNewEnv();
            }
            bool ret = true;
            Type *last_type = new SimpleType(TYPE_VOID);
            for (auto node : nodes)
            {
                ret &= node->typeCheck(symTable);
                Type *new_type;
                if (dynamic_cast<yyExpression *>(node) != nullptr)
                {
                    // Expression statement cant have return statement
                    // Dont use their type
                    new_type = new SimpleType(TYPE_VOID);
                }
                else
                {
                    // all other statements can have return statement(s),
                    // their `my_type` will be set to the type of the return statement
                    new_type = node->my_type;
                }
                assert(new_type != nullptr);
                Type *merged_type = merge_statement_types(last_type, new_type);
                if (merged_type == nullptr)
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in compound statement: "
                              << "Prev statement's type: " << last_type->typeStr() << " Current Statement's type: " << new_type->typeStr() << std::endl;
                    ret = false;
                    break;
                }
                else
                {
                    last_type = merged_type;
                }
            }
            my_type = last_type;
            if (!dont_create_new_env)
            {
                symTable->popEnv();
            }
            return ret;
        }
    };

    class yyFunctionDefinition : public yyAST
    {
    public:
        std::string name()
        {
            return "yyFunctionDefinition";
        }

        yyFunctionDefinition(yyAST *declSpecs, yyAST *declarator, yyAST *code)
        {
            nodes.push_back(declSpecs);
            nodes.push_back(declarator);
            nodes.push_back(code);
        }

        bool envCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 3);
            ret &= nodes[0]->envCheck(symTable);

            // symTable->createNewEnv();
            yyDeclarator *decl = dynamic_cast<yyDeclarator *>(nodes[1]);
            assert(decl != nullptr);
            yyDirectDeclarator *directDecl = dynamic_cast<yyDirectDeclarator *>(decl->directDeclarator);
            assert(directDecl != nullptr);

            directDecl->dont_drop_env = true;
            // Invariant: directDecl of function will push a new env and won't pop it
            ret &= directDecl->envCheck(symTable);
            directDecl->dont_drop_env = false;

            yyCompoundStatement *body = dynamic_cast<yyCompoundStatement *>(nodes[2]);
            assert(body != nullptr);

            for (auto node : body->nodes)
            {
                ret &= node->envCheck(symTable);
            }
            symTable->popEnv();

            return ret;
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 3);

            yyDeclSpecifiers *declSpecs = dynamic_cast<yyDeclSpecifiers *>(nodes[0]);
            assert(declSpecs != nullptr);
            yyTypeSpecifier *typeSpec = declSpecs->getType();
            assert(typeSpec != nullptr);

            Type *type = new SimpleType(typeSpec->type);

            yyDeclarator *decl = dynamic_cast<yyDeclarator *>(nodes[1]);

            if (decl->pointers.size() > 0)
            {
                type = new PointerType(decl->pointers.size(), SimpleType(typeSpec->type));
            }

            yyDirectDeclarator *directDecl = dynamic_cast<yyDirectDeclarator *>(decl->directDeclarator);
            assert(directDecl != nullptr);
            assert(directDecl->nodes.size() == 2);
            yyIdentifier *idNode = dynamic_cast<yyIdentifier *>(directDecl->nodes[0]);
            assert(idNode != nullptr);
            std::string id = idNode->id;

            // function declarator
            symTable->createNewEnv();
            yyParameterList *params = dynamic_cast<yyParameterList *>(directDecl->nodes[1]);
            assert(params != nullptr);
            std::vector<Type *> paramTypes;
            for (auto param : params->nodes)
            {
                yyParameterDecl *paramDecl = dynamic_cast<yyParameterDecl *>(param);
                yyEllipsis *ellipsis = dynamic_cast<yyEllipsis *>(param);
                assert(paramDecl != nullptr || ellipsis != nullptr);

                if (ellipsis != nullptr)
                {
                    // ellipsis = void*
                    paramTypes.push_back(new PointerType(1, SimpleType(TYPE_VOID)));
                }
                else
                {
                    assert(paramDecl->nodes.size() == 2);
                    yyDeclSpecifiers *paramDeclSpecs = dynamic_cast<yyDeclSpecifiers *>(paramDecl->nodes[0]);
                    assert(paramDeclSpecs != nullptr);
                    yyTypeSpecifier *paramTypeSpec = paramDeclSpecs->getType();
                    assert(paramTypeSpec != nullptr);
                    Type *paramType = new SimpleType(paramTypeSpec->type);
                    yyDeclarator *paramDeclr = dynamic_cast<yyDeclarator *>(paramDecl->nodes[1]);
                    assert(paramDeclr != nullptr);
                    if (paramDeclr->pointers.size() > 0)
                    {
                        paramType = new PointerType(paramDeclr->pointers.size(), SimpleType(paramTypeSpec->type));
                    }
                    paramTypes.push_back(paramType);

                    yyDirectDeclarator *paramDirectDecl = dynamic_cast<yyDirectDeclarator *>(paramDeclr->directDeclarator);
                    assert(paramDirectDecl != nullptr);
                    assert(paramDirectDecl->nodes.size() == 1);

                    yyIdentifier *paramIdNode = dynamic_cast<yyIdentifier *>(paramDirectDecl->nodes[0]);
                    assert(paramIdNode != nullptr);
                    std::string paramId = paramIdNode->id;

                    paramDecl->my_type = paramType;

                    symTable->addToEnv(paramId, paramDecl);
                }
            }

            idNode->my_type = new FunctionType(paramTypes, type);

            // Add function to symbol table, for recursive function calls in the body
            symTable->addToEnv(idNode->id, idNode);

            yyCompoundStatement *body = dynamic_cast<yyCompoundStatement *>(nodes[2]);
            assert(body != nullptr);

            body->dont_create_new_env = true;
            ret &= body->typeCheck(symTable);

            symTable->popEnv();

            if (!body->my_type->equals(type))
            {
                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible return type in function definition: "
                          << "Expected: " << type->typeStr() << " but got " << body->my_type->typeStr() << std::endl;
                ret = false;
            }

            symTable->addToEnv(idNode->id, idNode);

            return ret;
        }
    };

    class yyStatement : public yyAST
    {
    public:
        std::string name()
        {
            return "yyStatement";
        }
        // default impl. of envCheck will work.
    };

    class yyConditionalExpression : public yyAST
    {
    public:
        std::string name()
        {
            return "yyConditionalExpression";
        }
        // default impl. of envCheck will work.
    };

    enum BinaryOp
    {
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

    enum AssignOp
    {
        ASSIGN,
        MUL_ASSIGN,
        DIV_ASSIGN,
        MOD_ASSIGN,
        ADD_ASSIGN,
        SUB_ASSIGN,
        LEFT_ASSIGN,
        RIGHT_ASSIGN,
        AND_ASSIGN,
        XOR_ASSIGN,
        OR_ASSIGN,
    };

    class yyAssignmentExpression : public yyAST
    {
    public:
        std::string name()
        {
            return "yyAssignmentExpression";
        }
        AssignOp binaryOp;
        yyAssignmentExpression(yyAST *lhs, AssignOp op, yyAST *rhs)
        {
            binaryOp = op;
            nodes.push_back(lhs);
            nodes.push_back(rhs);
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 2);
            yyAST *lhs = nodes[0];
            yyAST *rhs = nodes[1];

            ret &= lhs->typeCheck(symTable);
            ret &= rhs->typeCheck(symTable);

            if (!lhs->my_type->equals(rhs->my_type))
            {
                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in assignment: "
                          << "Expected: " << lhs->my_type->typeStr() << " but got " << rhs->my_type->typeStr() << std::endl;
                ret = false;
            }

            this->my_type = lhs->my_type;

            return ret;
        }
    };

    class yyBinaryOp : public yyAST
    {
    public:
        BinaryOp binaryOp;

        std::string name()
        {
            switch (binaryOp)
            {
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

        yyBinaryOp(yyAST *left, BinaryOp binOp, yyAST *right)
        {
            nodes.push_back(left);
            nodes.push_back(right);
            binaryOp = binOp;
        }

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 2);
            yyAST *left = nodes[0];
            yyAST *right = nodes[1];

            if (binaryOp != BinaryOp::FUNC_CALL)
            {
                ret &= left->typeCheck(symTable);
                ret &= right->typeCheck(symTable);

                if (!left->my_type->equals(right->my_type))
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in binary operation: "
                              << "Expected: " << left->my_type->typeStr() << " but got " << right->my_type->typeStr() << std::endl;
                    ret = false;
                }
                else
                {
                    // TODO : Currently not checking str + str, bool + bool etc..
                    //  `t + `t is passes the type check as of now
                    const static std::set<BinaryOp> logicalOperations = {
                        BinaryOp::LOGICAL_OR,
                        BinaryOp::LOGICAL_AND,
                        BinaryOp::EQUAL,
                        BinaryOp::NOT_EQUAL,
                        BinaryOp::GT,
                        BinaryOp::GTE,
                        BinaryOp::LT,
                        BinaryOp::LTE};

                    if (logicalOperations.count(binaryOp) != 0)
                    {
                        this->my_type = new SimpleType(TYPE_BOOL);
                    }
                    else
                    {
                        this->my_type = left->my_type;
                    }
                }
            }
            else
            {
                // function call
                ret &= left->typeCheck(symTable);
                FunctionType *funcType = dynamic_cast<FunctionType *>(left->my_type);
                assert(funcType != nullptr);
                ret &= right->typeCheck(symTable);

                bool containsEllipsis = false;

                for (auto param : funcType->paramTypes)
                {
                    if (param->equals(new SimpleType(TYPE_ELLIPSIS)))
                    {
                        containsEllipsis = true;
                        break;
                    }
                }

                if (!containsEllipsis && funcType->paramTypes.size() != right->nodes.size() ||
                    (containsEllipsis && right->nodes.size() < (funcType->paramTypes.size() - 1)))
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible number of arguments in function call: "
                              << "Expected: " << funcType->paramTypes.size() << " but got " << right->nodes.size() << std::endl;
                    ret = false;
                }
                else
                {
                    if (!containsEllipsis)
                    {
                        for (int i = 0; i < funcType->paramTypes.size(); i++)
                        {
                            if (!funcType->paramTypes[i]->equals(right->nodes[i]->my_type))
                            {
                                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in function call: "
                                          << "Expected: " << funcType->paramTypes[i]->typeStr() << " but got " << right->nodes[i]->my_type->typeStr() << std::endl;
                                ret = false;
                            }
                        }
                    }
                    else
                    {
                        for (int i = 0; i < funcType->paramTypes.size() - 1; i++)
                        {
                            if (!funcType->paramTypes[i]->equals(right->nodes[i]->my_type))
                            {
                                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in function call: "
                                          << "Expected: " << funcType->paramTypes[i]->typeStr() << " but got " << right->nodes[i]->my_type->typeStr() << std::endl;
                                ret = false;
                            }
                        }
                    }
                }
                this->my_type = funcType->returnType;
            }
            return ret;
        }
    };

    enum UnaryOp
    {
        PL,
        NEG,
        PRE_INC,
        PRE_DEC,
        POST_INC,
        POST_DEC,
        NOT,
        LOGICAL_NOT
    };

    class yyUnaryOp : public yyAST
    {
    public:
        UnaryOp unaryOp;

        std::string name()
        {
            switch (unaryOp)
            {
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

        yyUnaryOp(UnaryOp op, yyAST *opr)
        {
            unaryOp = op;
            nodes.push_back(opr);
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 1);
            if (unaryOp == LOGICAL_NOT) // Logical not
            {
                ret &= nodes[0]->typeCheck(symTable);
                if (!nodes[0]->my_type->equals(new SimpleType(TYPE_BOOL)))
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in unary operation: "
                              << "Expected: " << (new SimpleType(TYPE_BOOL))->typeStr() << " but got " << nodes[0]->my_type->typeStr() << std::endl;
                    ret = false;
                }
                else
                {
                    this->my_type = new SimpleType(TYPE_BOOL);
                }
            }
            else if (unaryOp == NOT) // Bitwise not
            {
                ret &= nodes[0]->typeCheck(symTable);
                if (!nodes[0]->my_type->equals(new SimpleType(TYPE_INT)))
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in unary operation: "
                              << "Expected: " << (new SimpleType(TYPE_INT))->typeStr() << " but got " << nodes[0]->my_type->typeStr() << std::endl;
                    ret = false;
                }
                else
                {
                    this->my_type = new SimpleType(TYPE_INT);
                }
            }
            else // arithmetic
            {
                ret &= nodes[0]->typeCheck(symTable);
                if (!nodes[0]->my_type->equals(new SimpleType(TYPE_INT)))
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in unary operation: "
                              << "Expected: " << (new SimpleType(TYPE_INT))->typeStr() << " but got " << nodes[0]->my_type->typeStr() << std::endl;
                    ret = false;
                }
                else
                {
                    this->my_type = new SimpleType(TYPE_INT);
                }
            }
            return ret;
        }
    };

    class yyArgumentExpressionList : public yyAST
    {
    public:
        std::string name()
        {
            return "yyArgumentExpressionList";
        }
        yyArgumentExpressionList(){}; // empty list
    };

    class yyJumpStatement : public yyAST
    {
    public:
        std::string name()
        {
            return "yyEllipsis";
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            assert(false); // invariant: should never be called
            return true;
        }
    };

    class yyReturnStatement : public yyAST
    {
    public:
        std::string name()
        {
            return "yyReturnStatement";
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            assert(nodes.size() == 1);
            bool res = nodes[0]->typeCheck(symTable);
            my_type = nodes[0]->my_type;
            return res;
        }
    };

    class yyIfThenElse : public yyAST
    {
    public:
        std::string name()
        {
            return "yyIfThenElse";
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool res = true;
            assert(nodes.size() == 2 || nodes.size() == 3);

            // if-then
            res &= nodes[0]->typeCheck(symTable);
            if (!nodes[0]->my_type->equals(new SimpleType(TYPE_BOOL)))
            {
                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in if-then: "
                          << "Expected: " << (new SimpleType(TYPE_BOOL))->typeStr() << " but got " << nodes[0]->my_type->typeStr() << std::endl;
                res = false;
            }

            if (nodes.size() == 2)
            {
                res &= nodes[1]->typeCheck(symTable);
                my_type = nodes[1]->my_type;
            }
            else
            {
                // if-then-else
                res &= nodes[1]->typeCheck(symTable);
                res &= nodes[2]->typeCheck(symTable);
                Type *type1 = nodes[1]->my_type;
                Type *type2 = nodes[2]->my_type;
                Type *merged_type = merge_statement_types(type1, type2);
                if (merged_type == nullptr)
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in if-then-else: "
                              << "Got: " << type1->typeStr() << " and " << type2->typeStr() << " but expected both to be of the same type" << std::endl;
                }
                else
                {
                    my_type = merged_type;
                }
            }
            return res;
        }
    };

    class yyWhileLoop : public yyAST
    {
    public:
        std::string name()
        {
            return "yyWhileLoop";
        }
        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool res = true;
            assert(nodes.size() == 2);

            // while
            res &= nodes[0]->typeCheck(symTable);
            if (!nodes[0]->my_type->equals(new SimpleType(TYPE_BOOL)))
            {
                std::cerr << "[Line No " << this->line_no << "] Error: Incompatible types in while: "
                          << "Expected: " << (new SimpleType(TYPE_BOOL))->typeStr() << " but got " << nodes[0]->my_type->typeStr() << std::endl;
                res = false;
            }

            // body
            res &= nodes[1]->typeCheck(symTable);
            my_type = nodes[1]->my_type;
            return res;
        }
    };
}
#endif
