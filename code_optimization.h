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
#include <vector>
#include <iostream>
#include <unordered_map>
#include <set>
#include <memory>

using namespace llvm;

struct CodeOptContext
{
    std::unique_ptr<LLVMContext> context;
    std::unique_ptr<Module> module;
    std::unique_ptr<IRBuilder<>> builder;
    CodeOptContext(LLVMContext *context,
                   Module *module,
                   IRBuilder<> *builder)
        : context(std::move(context)),
          module(std::move(module)), builder(std::move(builder)) {}
};

void optimize(CodeOptContext *codeOptContext);