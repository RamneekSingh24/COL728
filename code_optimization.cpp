#include "code_optimization.h"

#include "llvm/IR/Dominators.h"

// mem2reg includes dead code removal
static bool mem2reg(CodeOptContext *codeOptContext);
static bool constantFolding(CodeOptContext *codeOptContext);

void optimize(CodeOptContext *codeOptContext)
{

    mem2reg(codeOptContext);
    constantFolding(codeOptContext);
}

static bool removeDeadStores(Function *function, CodeOptContext *codeOptContext)
{

    bool changed = false;

    std::vector<std::pair<AllocaInst *, std::vector<Instruction *>>> allocaInstructions;

    for (auto bb = function->begin(); bb != function->end(); bb++)
    {
        for (auto instr = bb->begin(); instr != bb->end(); instr++)
        {
            if (isa<AllocaInst>(instr))
            {
                auto alloca = dyn_cast<AllocaInst>(instr);
                allocaInstructions.push_back(std::make_pair(alloca, std::vector<Instruction *>()));
            }
        }
    }

    std::vector<AllocaInst *> usefulAllocaInstructions;

    for (auto instr : allocaInstructions)
    {

        bool isUseful = false;

        for (auto bb = function->begin(); bb != function->end(); bb++)
        {
            for (auto otherInstr = bb->begin(); otherInstr != bb->end(); otherInstr++)
            {

                if (isa<StoreInst>(otherInstr))
                {
                    auto store = dyn_cast<StoreInst>(otherInstr);
                    if (store->getPointerOperand() == instr.first)
                    {
                        instr.second.push_back(store);
                    }
                }
                else if (isa<LoadInst>(otherInstr))
                {
                    auto load = dyn_cast<LoadInst>(otherInstr);
                    if (load->getPointerOperand() == instr.first)
                    {
                        instr.second.push_back(load);
                        isUseful = true;
                    }
                }
            }
        }
        if (!isUseful)
        {
            for (auto i : instr.second)
            {
                i->eraseFromParent();
                changed = true;
            }
            instr.first->eraseFromParent();
        }

        if (verifyFunction(*function, &errs()))
        {
            codeOptContext->module->print(errs(), nullptr);
            std::cerr << "Compilation Failed... Aborting.." << std::endl;
            exit(1);
            return changed;
        }
    }

    return changed;
}

static bool removeDeadInstructions(Function *function, CodeOptContext *codeOptContext)
{

    bool changed = false;
    for (auto bb = function->begin(); bb != function->end(); bb++)
    {
        auto instr = bb->begin();
        while (instr != bb->end())
        {
            if (instr->isTerminator())
            {
                instr++;
                continue;
            }
            if (isa<StoreInst>(instr))
            {
                instr++;
                continue;
            }
            if (isa<CallInst>(instr) || isa<InvokeInst>(instr))
            {
                instr++;
                continue;
            }

            if (instr->getNumUses() == 0)
            {
                instr = instr->eraseFromParent();
                changed = true;
            }
            else
            {
                instr++;
            }
        }
    }

    if (verifyFunction(*function, &errs()))
    {
        codeOptContext->module->print(errs(), nullptr);
        std::cerr << "Compilation Failed... Aborting.." << std::endl;
        exit(1);
        return changed;
    }
    return changed;
}

static bool promoteAllocas(Function *function, CodeOptContext *codeOptContext)
{

    bool changed = false;

    std::vector<AllocaInst *> allocaInstructions;

    for (auto bb = function->begin(); bb != function->end(); bb++)
    {
        for (auto instr = bb->begin(); instr != bb->end(); instr++)
        {
            if (isa<AllocaInst>(instr))
            {
                auto alloca = dyn_cast<AllocaInst>(instr);
                allocaInstructions.push_back(alloca);
            }
        }
    }

    for (auto alloca : allocaInstructions)
    {

        auto DT = DominatorTree(*function);

        std::vector<LoadInst *> loadUses;
        std::vector<StoreInst *> storeUses;
        for (auto user : alloca->users())
        {
            if (isa<LoadInst>(user))
            {
                loadUses.push_back(dyn_cast<LoadInst>(user));
            }
            else if (isa<StoreInst>(user))
            {
                storeUses.push_back(dyn_cast<StoreInst>(user));
            }
        }

        // std::cout << "Alloca: " << alloca->getName().str() << " " << storeUses.size() << " " << loadUses.size() << std::endl;

        if (storeUses.size() == 0 || loadUses.size() == 0)
        {
            continue;
        }

        if (storeUses.size() == 1)
        {
            // if only one store usage, replace the all the loads **after the store** with the store value
            // note that in cases like
            // int x; y = x; x = 5;,  there is only one store, but y takes the un-stored value of x

            auto store = storeUses[0];

            auto storeBB = store->getParent();
            bool someLoadBeforeStore = false;
            for (auto load : loadUses)
            {
                auto loadBB = load->getParent();

                // if loadBB is in same bb as store
                if (loadBB == storeBB)
                {
                    // if load is after store
                    if (DT.dominates(store, load))
                    {
                        load->replaceAllUsesWith(store->getValueOperand());
                        load->eraseFromParent();
                        changed = true;
                    }
                    else
                    {
                        // if load is before store
                        // do nothing
                        someLoadBeforeStore = true;
                    }
                }
                else
                {
                    // if loadBB is after storeBB
                    if (DT.dominates(storeBB, loadBB))
                    {
                        load->replaceAllUsesWith(store->getValueOperand());
                        load->eraseFromParent();
                        changed = true;
                    }
                    else
                    {
                        // if loadBB is before storeBB
                        // do nothing
                        someLoadBeforeStore = true;
                    }
                }
            }
            if (!someLoadBeforeStore)
            {
                store->eraseFromParent();
                alloca->eraseFromParent();
                changed = true;
            }
            // function->print(errs(), nullptr);
            continue;
        }

        bool allLoadStoresInSameBB = true;
        assert(storeUses.size() > 1);

        auto firstStoreBB = storeUses[0]->getParent();
        for (auto load : loadUses)
        {
            if (load->getParent() != firstStoreBB)
            {
                allLoadStoresInSameBB = false;
                break;
            }
        }

        for (auto store : storeUses)
        {
            if (store->getParent() != firstStoreBB)
            {
                allLoadStoresInSameBB = false;
                break;
            }
        }

        // all load stores in same bb
        if (allLoadStoresInSameBB)
        {
            StoreInst *prevStore = nullptr;
            auto instr = firstStoreBB->begin();
            while (instr != firstStoreBB->end())
            {
                if (isa<StoreInst>(instr))
                {
                    auto store = dyn_cast<StoreInst>(instr);
                    if (store->getPointerOperand() == alloca)
                    {
                        prevStore = store;
                    }
                }
                if (isa<LoadInst>(instr))
                {
                    auto load = dyn_cast<LoadInst>(instr);
                    if (load->getPointerOperand() == alloca)
                    {
                        if (prevStore != nullptr)
                        {
                            load->replaceAllUsesWith(prevStore->getValueOperand());
                            instr = instr->eraseFromParent();
                            continue;
                            changed = true;
                        }
                    }
                }
                instr++;
            }
        }
    }

    if (verifyFunction(*function, &errs()))
    {
        codeOptContext->module->print(errs(), nullptr);
        std::cerr << "Compilation Failed... Aborting.." << std::endl;
        exit(1);
        return changed;
    }
    return changed;
}

static bool mem2reg(CodeOptContext *codeOptContext)
{
    auto module = codeOptContext->module.get();
    auto functions = module->functions();
    bool ret = false;
    for (auto it = functions.begin(); it != functions.end(); it++)
    {
        auto function = &*it;
        while (true)
        {
            bool changed = promoteAllocas(function, codeOptContext);
            changed = changed || removeDeadStores(function, codeOptContext);
            changed = changed || removeDeadInstructions(function, codeOptContext);
            if (!changed)
            {
                break;
            }
            else
            {
                ret = true;
            }
        }
    }
    return ret;
}

static bool propagateConstants(Function *function, CodeOptContext *codeOptContext)
{
    bool changed = false;
    for (auto bb = function->begin(); bb != function->end(); bb++)
    {
        for (auto instr = bb->begin(); instr != bb->end(); instr++)
        {
            if (isa<BinaryOperator>(instr))
            {
                auto binOp = dyn_cast<BinaryOperator>(instr);
                if (isa<ConstantInt>(binOp->getOperand(0)) && isa<ConstantInt>(binOp->getOperand(1)))
                {
                    auto op0 = dyn_cast<ConstantInt>(binOp->getOperand(0));
                    auto op1 = dyn_cast<ConstantInt>(binOp->getOperand(1));
                    auto result = ConstantInt::get(op0->getType(), 0);
                    switch (binOp->getOpcode())
                    {
                    case Instruction::Add:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() + op1->getSExtValue());
                        break;
                    case Instruction::Sub:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() - op1->getSExtValue());
                        break;
                    case Instruction::Mul:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() * op1->getSExtValue());
                        break;
                    case Instruction::SDiv:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() / op1->getSExtValue());
                        break;
                    case Instruction::SRem:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() % op1->getSExtValue());
                        break;
                    case Instruction::And:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() & op1->getSExtValue());
                        break;
                    case Instruction::Or:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() | op1->getSExtValue());
                        break;
                    case Instruction::Xor:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() ^ op1->getSExtValue());
                        break;
                    case Instruction::Shl:
                        result = ConstantInt::get(op0->getType(), op0->getSExtValue() << op1->getSExtValue());
                        break;
                    default:
                        assert(false && "Unimplemented op code");
                        break;
                    }

                    binOp->replaceAllUsesWith(result);
                    instr->eraseFromParent();
                    changed = true;
                    break;
                }
            }
        }
    }
    if (verifyFunction(*function, &errs()))
    {
        codeOptContext->module->print(errs(), nullptr);
        std::cerr << "Compilation Failed... Aborting.." << std::endl;
        exit(1);
        return changed;
    }
    return changed;
}

static bool constantFolding(CodeOptContext *codeOptContext)
{
    auto module = codeOptContext->module.get();
    auto functions = module->functions();
    bool ret = false;
    for (auto it = functions.begin(); it != functions.end(); it++)
    {
        auto function = &*it;
        while (true)
        {
            bool changed = propagateConstants(function, codeOptContext);
            changed = changed || removeDeadInstructions(function, codeOptContext);
            if (!changed)
            {
                break;
            }
            else
            {
                ret = true;
            }
        }
    }
    return ret;
}