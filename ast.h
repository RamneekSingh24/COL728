#include <vector>
#include <iostream>
#include <unordered_map>
#include <set>
#include <memory>
#include "SymbolTable.h"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#ifndef AST_H
#define AST_H

// TODO: Compact the AST for codegen (remove unnecessary nodes)
// TODO: HINT : declare global variables / functions in header file as static. Not working !!
// recursive calls from this file a getting different values
// TODO: check no seg faults in case of err recovery
// TODO: unnecessary load of lhs in assignment operations: `t = b`

extern int yylineno;

using namespace llvm;

namespace ast
{

    struct CodeGenContext
    {
        std::unique_ptr<LLVMContext> context;
        std::unique_ptr<Module> module;
        std::unique_ptr<IRBuilder<>> builder;
        std::unordered_map<std::string, Value *> *stringLiterals;
        SymbolTable<llvm::Value *> *varTable;
        SymbolTable<llvm::Function *> *funcTable;

        CodeGenContext()
        {
            context = std::make_unique<LLVMContext>();
            module = std::make_unique<Module>("col728", *context);
            builder = std::make_unique<IRBuilder<>>(*context);
            stringLiterals = new std::unordered_map<std::string, Value *>();
            varTable = new SymbolTable<llvm::Value *>();
            funcTable = new SymbolTable<llvm::Function *>();
        }
    };

    // static auto context = std::make_unique<LLVMContext>();
    // static auto module = std::make_unique<Module>("col728", *context);
    // static auto builder = std::make_unique<IRBuilder<>>(*context);
    // static std::unordered_map<std::string, Value *> stringLiterals;

    // static SymbolTable<llvm::Value *> *varTable = new SymbolTable<llvm::Value *>();
    // static SymbolTable<llvm::Function *> *functionTable = new SymbolTable<llvm::Function *>();

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
        virtual llvm::Type *llvmType(CodeGenContext *context)
        {
            return nullptr;
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
        llvm::Type *llvmType(CodeGenContext *context)
        {
            switch (simpleType)
            {
            case TYPE_INT:
                return llvm::Type::getInt32Ty(*context->context);
            case TYPE_VOID:
                return llvm::Type::getVoidTy(*context->context);
            case TYPE_FLOAT:
                return llvm::Type::getFloatTy(*context->context);
            case TYPE_CHAR:
                return llvm::Type::getInt8Ty(*context->context);
            case TYPE_BOOL:
                return llvm::Type::getInt1Ty(*context->context);
            case TYPE_ELLIPSIS:
                return nullptr;
            }
            return nullptr;
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
        llvm::Type *llvmType(CodeGenContext *context)
        {
            llvm::Type *type = simpleType.llvmType(context);
            for (int i = 0; i < pointer_cnt; i++)
            {
                type = type->getPointerTo();
            }
            return type;
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

        llvm::FunctionType *llvmFuncType(CodeGenContext *context)
        {
            std::vector<llvm::Type *> paramTypesLLVM;
            bool containsEllipsis = false;
            for (auto type : paramTypes)
            {
                if (type->typeStr() == "...")
                {
                    containsEllipsis = true;
                }
                else
                {
                    auto llvm_type = type->llvmType(context);
                    assert(llvm_type != nullptr);
                    paramTypesLLVM.push_back(llvm_type);
                }
            }
            return llvm::FunctionType::get(returnType->llvmType(context), paramTypesLLVM, containsEllipsis);
        }

        llvm::Type *llvmType(CodeGenContext *context)
        {
            return llvmFuncType(context);
        }
    };

    // template <typename T>
    class yyAST
    {
    public:
        int line_no;

        bool dont_drop_env = false;

        Type *my_type = new SimpleType(TYPE_VOID);

        LLVMValueRef llvm_value = nullptr;

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

        virtual Value *codeGen(CodeGenContext *cgenContext)
        {
            for (auto node : nodes)
            {
                node->codeGen(cgenContext);
            }
            return nullptr;
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            auto varTable = cgenContext->varTable;
            auto funcTable = cgenContext->funcTable;

            varTable->createNewEnv();
            funcTable->createNewEnv();

            for (auto node : nodes)
            {
                node->codeGen(cgenContext);
            }
            return nullptr;
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
        // Base implementation of typeCheck will work.
        // Base implementation of codeGen will work.
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
        // Base implementation of typeCheck will work.
        // Base implementation of codeGen will work.
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
        // Base Implementation of typeCheck will work.
        // Base Implementation of codeGen will work.
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

        // at this stage we are storing all declared variables in the stack
        // the optimization stages will fix this.
        Value *codeGen(CodeGenContext *cgenContext)
        {
            auto stack_loc = cgenContext->varTable->getFromEnv(id);
            assert(stack_loc != nullptr); // errors should have been detected by semantic analysis.
            assert(my_type != nullptr);
            auto llvm_type = my_type->llvmType(cgenContext);
            return cgenContext->builder->CreateLoad(llvm_type, stack_loc, "local_var");
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            return ConstantInt::get(*(cgenContext->context), APInt(32, v, true));
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            return ConstantFP::get(*(cgenContext->context), APFloat(v));
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            auto stringLiterals = cgenContext->stringLiterals;
            auto builder = cgenContext->builder.get();
            auto module = cgenContext->module.get();
            if (stringLiterals->count(v) == 0)
            {
                stringLiterals->operator[](v) = builder->CreateGlobalStringPtr(v, "string_literal", 0, module);
            }
            return stringLiterals->operator[](v);
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

        std::string declID;
        Type *declType;
        bool isFunctionDecl = false;

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
                isFunctionDecl = false;
                idNode->my_type = type;
            }
            else
            {
                isFunctionDecl = true;
                // function declarator
                if (symTable->table.size() != 1)
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Function declaration must be global" << std::endl;
                }

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

            declID = idNode->id;
            declType = idNode->my_type;

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

        Value *codeGen(CodeGenContext *cgenContext)
        {
            auto varTable = cgenContext->varTable;
            auto functionTable = cgenContext->funcTable;
            assert(declType != nullptr);
            if (isFunctionDecl)
            {
                assert(varTable->table.size() == 1);      // should be at TU level
                assert(functionTable->table.size() == 1); // just for safety
                FunctionType *funcType = dynamic_cast<FunctionType *>(declType);
                assert(funcType != nullptr);
                auto llvmFuncType = funcType->llvmFuncType(cgenContext);
                Function *func = Function::Create(llvmFuncType, Function::ExternalLinkage, declID, cgenContext->module.get());
                functionTable->addToEnv(declID, func);
            }
            else
            {

                if (varTable->table.size() == 1) // global variable
                {

                    GlobalVariable *globalVar = new GlobalVariable(*(cgenContext->module), declType->llvmType(cgenContext), false,
                                                                   GlobalValue::ExternalLinkage, nullptr, declID);
                    varTable->addToEnv(declID, globalVar);
                }
                else
                {
                    // local variable
                    AllocaInst *alloca = cgenContext->builder->CreateAlloca(declType->llvmType(cgenContext), nullptr, declID);
                    varTable->addToEnv(declID, alloca);
                }
            }
            return nullptr;
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            if (nodes.size() == 0)
            {
                return nullptr;
            }
            Value *val;
            for (auto node : nodes)
            {
                val = node->codeGen(cgenContext);
            }
            return val;
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            if (!dont_create_new_env)
            {
                cgenContext->varTable->createNewEnv();
            }
            Value *val;
            for (auto node : nodes)
            {
                val = node->codeGen(cgenContext);
            }
            if (!dont_create_new_env)
            {
                cgenContext->varTable->popEnv();
            }
            return val;
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

        Type *declType;
        std::string declID;
        std::vector<std::string> argNames;

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
                    // Ellipsis using inside def not supported yet,
                    // so we don't allow it even in the argument list..
                    std::cerr << "Fatal: Ellipsis not supported in function definition yet.." << std::endl;
                    exit(1);
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
                    argNames.push_back(paramId);
                    paramDecl->my_type = paramType;

                    symTable->addToEnv(paramId, paramDecl);
                }
            }

            idNode->my_type = new FunctionType(paramTypes, type);

            declID = idNode->id;
            declType = idNode->my_type;

            // Add function to symbol table, for recursive function calls in the body
            symTable->addToEnv(idNode->id, idNode);

            yyCompoundStatement *body = dynamic_cast<yyCompoundStatement *>(nodes[2]);
            assert(body != nullptr);

            body->dont_create_new_env = true;
            ret &= body->typeCheck(symTable);
            body->dont_create_new_env = false;

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

        Value *codeGen(CodeGenContext *cgenContext)
        {

            auto varTable = cgenContext->varTable;
            auto functionTable = cgenContext->funcTable;
            assert(declType != nullptr);

            assert(varTable->table.size() == 1);      // should be at TU level
            assert(functionTable->table.size() == 1); // just for safety

            FunctionType *funcType = dynamic_cast<FunctionType *>(declType);
            assert(funcType != nullptr);
            auto llvmFuncType = funcType->llvmFuncType(cgenContext);
            Function *func = Function::Create(llvmFuncType, Function::ExternalLinkage, declID, cgenContext->module.get());

            for (size_t i = 0; i < argNames.size(); i++)
            {
                func->getArg(i)->setName(argNames[i]);
            }

            functionTable->addToEnv(declID, func);

            BasicBlock *bb = BasicBlock::Create(*cgenContext->context, "entry", func);
            cgenContext->builder->SetInsertPoint(bb);

            varTable->createNewEnv();

            for (auto &arg : func->args())
            {
                auto argName = arg.getName();
                AllocaInst *alloca = cgenContext->builder->CreateAlloca(arg.getType(), nullptr, argName);
                cgenContext->builder->CreateStore(&arg, alloca);
                varTable->addToEnv(argName.str(), alloca);
            }

            yyCompoundStatement *body = dynamic_cast<yyCompoundStatement *>(nodes[2]);
            assert(body != nullptr);

            body->dont_create_new_env = true;
            auto body_val = body->codeGen(cgenContext);
            body->dont_create_new_env = false;

            // add 'return void' only if the function's return type is void

            auto retType = dynamic_cast<SimpleType *>(funcType->returnType);

            if (retType != nullptr && retType->simpleType == TYPE_VOID)
            {
                cgenContext->builder->CreateRetVoid();
            }

            // remove dead instructions(including more terminals!) after the last terminal in each basic block..
            // if not done, then the verifyFunction complains: Terminal found in middle of Basic block
            // although the code is correct & runs on llvm interpreter
            for (auto bb = func->begin(); bb != func->end();)
            {

                bool terminal_found = false;
                auto i = bb->begin();
                while (i != bb->end())
                {
                    if (terminal_found)
                    {
                        i = i->eraseFromParent();
                    }
                    else
                    {
                        if (i->isTerminator())
                        {
                            terminal_found = true;
                        }
                        i++;
                    }
                }
                // remove dead bb's
                if (bb->empty())
                {
                    bb = bb->eraseFromParent();
                }
                else
                {
                    bb++;
                }
            }

            // we may not have a return statement in the function body
            // verifyFunction will detect it

            varTable->popEnv();

            if (verifyFunction(*func, &errs()))
            {
                std::cerr << "\n[Line No " << this->line_no << "] Fatal Error: Codegen for Function " << declID << " didn't pass LLVM's verifyFunction" << std::endl;
                std::cerr << "This most probably happened because function didn't have return statement in all control paths" << std::endl;
                std::cerr << "Printing module so far.." << std::endl;
                cgenContext->module->print(errs(), nullptr);
                std::cerr << "Compilation Failed... Aborting.." << std::endl;
                exit(1);
            }

            return body_val;
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

        Value *codeGenLHSAssign(CodeGenContext *CodeGenContext, yyAST *lhs)
        {
            // For now we only support direct assignment to variables
            // so the lhs should be an identifier
            yyIdentifier *id = dynamic_cast<yyIdentifier *>(lhs);
            assert(id != nullptr);
            auto varTable = CodeGenContext->varTable;
            auto var = varTable->getFromEnv(id->id);
            // var is a location in memory
            return var;
        }

        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 2);
            yyAST *lhs = nodes[0];
            yyAST *rhs = nodes[1];

            auto lhs_loc = codeGenLHSAssign(cgenContext, lhs);

            auto rhs_val = rhs->codeGen(cgenContext);

            assert(lhs_loc != nullptr && rhs_val != nullptr);
            Value *tmp = nullptr;

            if (binaryOp == ASSIGN)
            {
                cgenContext->builder->CreateStore(rhs_val, lhs_loc);
                return rhs_val;
            }

            auto lhs_val = lhs->codeGen(cgenContext); // value of lhs after evaluating rhs

            switch (binaryOp)
            {
            case ASSIGN:
                assert(false); // should not reach here
                break;
            case MUL_ASSIGN:
                tmp = cgenContext->builder->CreateMul(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case DIV_ASSIGN:
                tmp = cgenContext->builder->CreateSDiv(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case MOD_ASSIGN:
                tmp = cgenContext->builder->CreateSRem(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case ADD_ASSIGN:
                tmp = cgenContext->builder->CreateAdd(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case SUB_ASSIGN:
                tmp = cgenContext->builder->CreateSub(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case LEFT_ASSIGN:
                tmp = cgenContext->builder->CreateShl(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case RIGHT_ASSIGN:
                tmp = cgenContext->builder->CreateAShr(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case AND_ASSIGN:
                tmp = cgenContext->builder->CreateAnd(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case XOR_ASSIGN:
                tmp = cgenContext->builder->CreateXor(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            case OR_ASSIGN:
                tmp = cgenContext->builder->CreateOr(lhs_val, rhs_val);
                cgenContext->builder->CreateStore(tmp, lhs_loc);
                break;
            }
            return lhs_val;
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

        bool isFuncCall = false;

        std::string funcName;

        bool typeCheck(SymbolTable<yyAST *> *symTable)
        {
            bool ret = true;
            assert(nodes.size() == 2);
            yyAST *left = nodes[0];
            yyAST *right = nodes[1];

            if (binaryOp != BinaryOp::FUNC_CALL)
            {
                isFuncCall = false;
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
                isFuncCall = true;

                ret &= left->typeCheck(symTable);
                FunctionType *funcType = dynamic_cast<FunctionType *>(left->my_type);

                if (funcType == nullptr)
                {
                    std::cerr << "[Line No " << this->line_no << "] Error: Expected function type but got " << left->my_type->typeStr() << std::endl;
                    ret = false;
                }
                else
                {

                    ret &= right->typeCheck(symTable);

                    yyIdentifier *funcNameId = dynamic_cast<yyIdentifier *>(left);
                    assert(funcNameId != nullptr);

                    funcName = funcNameId->id;

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
            }
            return ret;
        }

        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 2);
            yyAST *left = nodes[0];
            yyAST *right = nodes[1];

            if (binaryOp != FUNC_CALL)
            {
                Value *lhs_val = left->codeGen(cgenContext);
                Value *rhs_val = right->codeGen(cgenContext);
                Value *tmp = nullptr;
                switch (binaryOp)
                {
                case BinaryOp::PLUS:
                    tmp = cgenContext->builder->CreateAdd(lhs_val, rhs_val, "addtmp");
                    break;
                case BinaryOp::MINUS:
                    tmp = cgenContext->builder->CreateSub(lhs_val, rhs_val, "subtmp");
                    break;
                case BinaryOp::MULT:
                    tmp = cgenContext->builder->CreateMul(lhs_val, rhs_val, "multmp");
                    break;
                case BinaryOp::DIV:
                    tmp = cgenContext->builder->CreateSDiv(lhs_val, rhs_val, "divtmp");
                    break;
                case BinaryOp::MOD:
                    tmp = cgenContext->builder->CreateSRem(lhs_val, rhs_val, "modtmp");
                    break;
                case BinaryOp::OR:
                    tmp = cgenContext->builder->CreateOr(lhs_val, rhs_val, "ortmp");
                    break;
                case BinaryOp::AND:
                    tmp = cgenContext->builder->CreateAnd(lhs_val, rhs_val, "andtmp");
                    break;
                case BinaryOp::XOR:
                    tmp = cgenContext->builder->CreateXor(lhs_val, rhs_val, "xortmp");
                    break;
                case BinaryOp::LSHIFT:
                    tmp = cgenContext->builder->CreateShl(lhs_val, rhs_val, "lshifttmp");
                    break;
                case BinaryOp::RSHIFT:
                    tmp = cgenContext->builder->CreateAShr(lhs_val, rhs_val, "rshifttmp");
                    break;
                case BinaryOp::GT:
                    tmp = cgenContext->builder->CreateICmpSGT(lhs_val, rhs_val, "gttmp");
                    break;
                case BinaryOp::GTE:
                    tmp = cgenContext->builder->CreateICmpSGE(lhs_val, rhs_val, "gtetmp");
                    break;
                case BinaryOp::LT:
                    tmp = cgenContext->builder->CreateICmpSLT(lhs_val, rhs_val, "lttmp");
                    break;
                case BinaryOp::LOGICAL_AND:
                    tmp = cgenContext->builder->CreateLogicalAnd(lhs_val, rhs_val, "andtmp");
                    break;
                case BinaryOp::LOGICAL_OR:
                    tmp = cgenContext->builder->CreateLogicalOr(lhs_val, rhs_val, "ortmp");
                    break;
                case BinaryOp::EQUAL:
                    tmp = cgenContext->builder->CreateICmpEQ(lhs_val, rhs_val, "eqtmp");
                    break;
                case BinaryOp::NOT_EQUAL:
                    tmp = cgenContext->builder->CreateICmpNE(lhs_val, rhs_val, "netmp");
                    break;
                case BinaryOp::LTE:
                    tmp = cgenContext->builder->CreateICmpSLE(lhs_val, rhs_val, "ltetmp");
                    break;
                case BinaryOp::FUNC_CALL:
                    assert(false);
                    break;
                }
                return tmp;
            }

            else
            {
                // func call
                auto func = cgenContext->funcTable->getFromEnv(funcName);
                assert(func != nullptr);

                std::vector<Value *> args;
                for (auto arg : right->nodes)
                {
                    args.push_back(arg->codeGen(cgenContext));
                }
                return cgenContext->builder->CreateCall(func, args, "calltmp");
            }
        }
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

        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 1);
            yyAST *opr = nodes[0];
            Value *opr_val = opr->codeGen(cgenContext);

            switch (unaryOp)
            {
            case UnaryOp::PL:
                return opr_val;
            case UnaryOp::NEG:
                return cgenContext->builder->CreateNeg(opr_val, "negtmp");
            case UnaryOp::NOT:
                return cgenContext->builder->CreateNot(opr_val, "nottmp");
            case UnaryOp::LOGICAL_NOT:
                return cgenContext->builder->CreateNot(opr_val, "logicalnottmp");
                // TODO Test above ^^^
            default:
                break;
            }
            // unary assignment operators

            yyIdentifier *idNode = dynamic_cast<yyIdentifier *>(opr);

            if (idNode == nullptr)
            {
                std::cerr << "[Line No " << this->line_no << "] Error: Invalid lvalue for unary assignment operator" << std::endl;
                // TODO: can move to type check phase
                std::cerr << "Fatal: "
                          << "Not supported error recovery for errors in codegen stage yet.." << std::endl;
                exit(1);
                return nullptr;
            }

            Value *opr_loc = cgenContext->varTable->getFromEnv(idNode->id);

            assert(opr_loc != nullptr);
            auto c1 = ConstantInt::get(*(cgenContext->context), APInt(32, 1, true));

            Value *tmp;

            switch (unaryOp)
            {
            case UnaryOp::PRE_INC:
                tmp = cgenContext->builder->CreateAdd(opr_val, c1, "preinctmp");
                cgenContext->builder->CreateStore(tmp, opr_loc);
                break;
            case UnaryOp::PRE_DEC:
                tmp = cgenContext->builder->CreateSub(opr_val, c1, "predectmp");
                cgenContext->builder->CreateStore(tmp, opr_loc);
                break;
            case UnaryOp::POST_INC:
                tmp = cgenContext->builder->CreateAdd(opr_val, c1, "postinctmp");
                cgenContext->builder->CreateStore(tmp, opr_loc);
                tmp = opr_val;
                break;
            case UnaryOp::POST_DEC:
                tmp = cgenContext->builder->CreateSub(opr_val, c1, "postdectmp");
                cgenContext->builder->CreateStore(tmp, opr_loc);
                tmp = opr_val;
                break;
            default:
                assert(false);
                break;
            }
            return tmp;
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
            return "yyJumpStatement";
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
        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 1);
            Value *ret_val = nodes[0]->codeGen(cgenContext);
            cgenContext->builder->CreateRet(ret_val);
            return ret_val;
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

        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 3 || nodes.size() == 2);
            if (nodes.size() == 3)
            {
                // if then else
                Value *cond = nodes[0]->codeGen(cgenContext);
                assert(cond != nullptr);
                Function *func = cgenContext->builder->GetInsertBlock()->getParent();
                BasicBlock *then_block = BasicBlock::Create(*(cgenContext->context), "then", func);
                BasicBlock *else_block = BasicBlock::Create(*(cgenContext->context), "else");
                BasicBlock *merge_block = BasicBlock::Create(*(cgenContext->context), "ifcont");

                cgenContext->builder->CreateCondBr(cond, then_block, else_block);

                cgenContext->builder->SetInsertPoint(then_block);

                auto then_val = nodes[1]->codeGen(cgenContext);

                cgenContext->builder->CreateBr(merge_block);
                then_block = cgenContext->builder->GetInsertBlock();

                func->getBasicBlockList().push_back(else_block);
                cgenContext->builder->SetInsertPoint(else_block);

                auto else_val = nodes[2]->codeGen(cgenContext);

                cgenContext->builder->CreateBr(merge_block);
                else_block = cgenContext->builder->GetInsertBlock();

                func->getBasicBlockList().push_back(merge_block);
                cgenContext->builder->SetInsertPoint(merge_block);

                return then_val;
            }
            else
            {
                // if then
                Value *cond = nodes[0]->codeGen(cgenContext);
                assert(cond != nullptr);
                Function *func = cgenContext->builder->GetInsertBlock()->getParent();
                BasicBlock *then_block = BasicBlock::Create(*(cgenContext->context), "then", func);
                BasicBlock *merge_block = BasicBlock::Create(*(cgenContext->context), "ifcont");

                cgenContext->builder->CreateCondBr(cond, then_block, merge_block);

                cgenContext->builder->SetInsertPoint(then_block);

                auto then_val = nodes[1]->codeGen(cgenContext);

                cgenContext->builder->CreateBr(merge_block);

                func->getBasicBlockList().push_back(merge_block);
                cgenContext->builder->SetInsertPoint(merge_block);

                return then_val;
            }
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

        Value *codeGen(CodeGenContext *cgenContext)
        {
            assert(nodes.size() == 2);
            Function *func = cgenContext->builder->GetInsertBlock()->getParent();
            BasicBlock *cond_block = BasicBlock::Create(*(cgenContext->context), "cond", func);
            BasicBlock *body_block = BasicBlock::Create(*(cgenContext->context), "body");
            BasicBlock *merge_block = BasicBlock::Create(*(cgenContext->context), "whilecont");

            cgenContext->builder->CreateBr(cond_block);

            cgenContext->builder->SetInsertPoint(cond_block);

            Value *cond = nodes[0]->codeGen(cgenContext);
            assert(cond != nullptr);
            cgenContext->builder->CreateCondBr(cond, body_block, merge_block);

            func->getBasicBlockList().push_back(body_block);
            cgenContext->builder->SetInsertPoint(body_block);

            auto body_val = nodes[1]->codeGen(cgenContext);

            cgenContext->builder->CreateBr(cond_block);
            body_block = cgenContext->builder->GetInsertBlock();

            func->getBasicBlockList().push_back(merge_block);
            cgenContext->builder->SetInsertPoint(merge_block);

            return body_val;
        }
    };

}
#endif
