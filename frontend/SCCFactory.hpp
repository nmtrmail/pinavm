#ifndef _SCCFACTORY_HPP
#define _SCCFACTORY_HPP

#include <map>

#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"

#include "SCConstructs/SCConstructHandler.hpp"


struct Process;

struct SCCFactory {

      private:
	std::map < Function *, SCConstructHandler * >scchandlers;
	std::map <Instruction *, std::map<Process*, SCConstruct *> > scc;

      public:
	SCCFactory(SCJit * jit);
	~SCCFactory();
	bool handle(Process* proc, llvm::Function * fct, BasicBlock * bb, Instruction* callInst, Function* calledFunction);
	std::map <Instruction *, std::map<Process*, SCConstruct *> >* getConstructs();
	bool handlerExists(llvm::Function * fct, BasicBlock * bb, Instruction* callInst, Function* calledFunction);
};

#endif
