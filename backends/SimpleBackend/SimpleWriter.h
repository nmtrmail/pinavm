#ifndef SIMPLEWRITER_H
#define SIMPLEWRITER_H

#include "SimpleTargetMachine.h"

#include "llvm/CallingConv.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/TypeSymbolTable.h"
#include "llvm/Intrinsics.h"
#include "llvm/IntrinsicInst.h"
#include "llvm/InlineAsm.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/ConstantsScanner.h"
#include "llvm/Analysis/FindUsedTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetRegistry.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/InstVisitor.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/System/Host.h"
#include "llvm/Config/config.h"

#include <map>
#include <vector>
#include <algorithm>
#include <sstream>
#include <string>
#include <iostream>
#include <utility>

#include "Frontend.hpp"

struct SCConstruct;
struct SCCFactory;
struct SCElab;

using namespace llvm;

enum SpecialGlobalClass {
  NotSpecial = 0,
  GlobalCtors, GlobalDtors,
  NotPrinted
};

/*
  MM: see PromelaBackend.h for my remark about rtti
// Borrowed from CBEMCAsmInfo in CBackend.cpp
class SimpleBEMCAsmInfo : public MCAsmInfo {
public:
	SimpleBEMCAsmInfo() {
		this->GlobalPrefix = "";
		this->PrivateGlobalPrefix = "";
	}
};
*/

/// SimpleWriter - This class is the main chunk of code that converts an LLVM
/// module to a "Simple" translation unit.
class SimpleWriter : public ModulePass, public InstVisitor<SimpleWriter> {
  formatted_raw_ostream &Out;
  IntrinsicLowering *IL;
  Mangler *Mang;
  LoopInfo *LI;
  const Module *TheModule;
  const MCAsmInfo* TAsm;
  const TargetData* TD;
  std::map<const Type *, std::string> TypeNames;
  std::map<const ConstantFP *, unsigned> FPConstantMap;
  std::set<Function*> intrinsicPrototypesAlreadyGenerated;
  std::set<const Argument*> ByValParams;
  unsigned FPCounter;
  unsigned OpaqueCounter;
  DenseMap<const Value*, unsigned> AnonValueNumbers;
  unsigned NextAnonValueNumber;
  SCCFactory* sccfactory;
  SCElab* elab;
  bool relativeClocks;
  bool eventsAsBool;

public:
  static char ID;
  explicit SimpleWriter(Frontend* fe, formatted_raw_ostream &o, bool useRelativeClocks, bool encodeEventsAsBool)
    : ModulePass(&ID), Out(o), IL(0), Mang(0), LI(0), 
      TheModule(0), TAsm(0), TD(0), OpaqueCounter(0), NextAnonValueNumber(0) {
    FPCounter = 0;
    this->sccfactory = fe->getConstructs();
    this->elab = fe->getElab();
    this->relativeClocks = useRelativeClocks;
    this->eventsAsBool = encodeEventsAsBool;
  }
  explicit SimpleWriter(formatted_raw_ostream &o)
    : ModulePass(&ID), Out(o), IL(0), Mang(0), LI(0), 
      TheModule(0), TAsm(0), TD(0), OpaqueCounter(0), NextAnonValueNumber(0) {
    FPCounter = 0;
  }

  const char *getPassName() const;
  void getAnalysisUsage(AnalysisUsage &AU) const;
  bool runOnModule(Module &M);

  raw_ostream &printType(formatted_raw_ostream &Out,
			 const Type *Ty, 
			 bool isSigned = false,
			 const std::string &VariableName = "",
			 bool IgnoreName = false,
			 const AttrListPtr &PAL = AttrListPtr());
  std::ostream &printType(std::ostream &Out, const Type *Ty, 
			  bool isSigned = false,
			  const std::string &VariableName = "",
			  bool IgnoreName = false,
			  const AttrListPtr &PAL = AttrListPtr());
  raw_ostream &printSimpleType(formatted_raw_ostream &Out,
			       const Type *Ty, 
			       bool isSigned, 
			       const std::string &NameSoFar = "");
  std::ostream &printSimpleType(std::ostream &Out, const Type *Ty, 
				bool isSigned, 
				const std::string &NameSoFar = "");

  void printStructReturnPointerFunctionType(formatted_raw_ostream &Out,
					    const AttrListPtr &PAL,
					    const PointerType *Ty);

  /// writeOperandDeref - Print the result of dereferencing the specified
  /// operand with '*'.  This is equivalent to printing '*' then using
  /// writeOperand, but avoids excess syntax in some cases.
  void writeOperandDeref(Value *Operand);
    
  void writeOperand(Value *Operand, bool Static = false);
  void writeInstComputationInline(Instruction &I);
  void writeOperandInternal(Value *Operand, bool Static = false);
  void writeOperandWithCast(Value* Operand, unsigned Opcode);
  void writeOperandWithCast(Value* Operand, const ICmpInst &I);
  bool writeInstructionCast(const Instruction &I);

  void writeMemoryAccess(Value *Operand, const Type *OperandType,
			 bool IsVolatile, unsigned Alignment);

  private :
    std::string InterpretASMConstraint(InlineAsm::ConstraintInfo& c);

  void lowerIntrinsics(Function &F);

  void printModule(Module *M);
  void fillModuleTypes(const TypeSymbolTable &ST);
  void printFloatingPointConstants(Function &F);
  void printFloatingPointConstants(const Constant *C);
  void printFunctionSignature(const Function *F, bool Prototype);
  void printReturnVariables(const Function* F);

  void printFunction(Function &);
  void printBasicBlock(BasicBlock *BB);
  void printLoop(Loop *L);

  void printCast(unsigned opcode, const Type *SrcTy, const Type *DstTy);
  void printConstant(Constant *CPV, bool Static);
  void printConstantWithCast(Constant *CPV, unsigned Opcode);
  bool printConstExprCast(const ConstantExpr *CE, bool Static);
  void printConstantArray(ConstantArray *CPA, bool Static);
  void printConstantVector(ConstantVector *CV, bool Static);

  /// isAddressExposed - Return true if the specified value's name needs to
  /// have its address taken in order to get a C value of the correct type.
  /// This happens for global variables, byval parameters, and direct allocas.
  bool isAddressExposed(const Value *V) const;
        
  // Instruction visitation functions
  friend class InstVisitor<SimpleWriter>;

  void visitReturnInst(ReturnInst &I);
  void visitBranchInst(BranchInst &I);
  void visitSwitchInst(SwitchInst &I);
  void visitInvokeInst(InvokeInst &I);
  void visitUnwindInst(UnwindInst &I);
  void visitUnreachableInst(UnreachableInst &I);

  void visitPHINode(PHINode &I);
  void visitBinaryOperator(Instruction &I);
  void visitICmpInst(ICmpInst &I);
  void visitFCmpInst(FCmpInst &I);

  void visitCastInst (CastInst &I);
  void visitSelectInst(SelectInst &I);

  void printCodingGlobals();
  void printSelectClock();
  void printWaitTimePrimitive();
  void printNotifyPrimitive();
  void printWaitEventPrimitive();
  void printPrimitives();
  void printGlobalVariables(Mangler* mang);
  void printProcesses();

  void visitSCConstruct(SCConstruct * scc);
  void visitCallInst (CallInst &I);
  void visitInlineAsm(CallInst &I);
  bool visitBuiltinCall(CallInst &I, Intrinsic::ID ID, bool &WroteCallee);

  void visitAllocaInst(AllocaInst &I);
  void visitLoadInst  (LoadInst   &I);
  void visitStoreInst (StoreInst  &I);
  void visitGetElementPtrInst(GetElementPtrInst &I);
  void visitVAArgInst (VAArgInst &I);

  void visitInsertElementInst(InsertElementInst &I);
  void visitExtractElementInst(ExtractElementInst &I);
  void visitShuffleVectorInst(ShuffleVectorInst &SVI);

  void visitInsertValueInst(InsertValueInst &I);
  void visitExtractValueInst(ExtractValueInst &I);

  void visitInstruction(Instruction &I);

  void outputLValue(Instruction *I);

  bool isGotoCodeNecessary(BasicBlock *From, BasicBlock *To);
  void printPHICopiesForSuccessor(BasicBlock *CurBlock,
				  BasicBlock *Successor, unsigned Indent);
  void printBranchToBlock(BasicBlock *CurBlock, BasicBlock *SuccBlock,
			  unsigned Indent);
  void printGEPExpression(Value *Ptr, gep_type_iterator I,
			  gep_type_iterator E, bool Static);

  std::string GetValueName(const Value *Operand);
  
  void fillDependencies(const Function* F,
		  std::string prefix,
		  std::vector<std::pair<std::string, const Type*> >* args,
		  std::vector<std::pair<std::string, const Type*> >* ret);
  
  void insertAllFields(std::vector<std::pair<std::string, const Type*> >* deps,
		  std::map<std::string, const Type*>* allDepsByName,
		  std::string parentName,
		  const StructType* structType);
  void addVectors(std::vector<std::pair<std::string, const Type*> >* from,
		  std::vector<std::pair<std::string, const Type*> >* to);
  bool isSystemCType(const Type* ty);
  void getValueDependencies(Value* value,
			  std::string prefix,
			  std::vector<std::pair<std::string, const Type*> >* args,
			  std::vector<std::pair<std::string, const Type*> >* ret,
			  std::map<Value*, std::string>* allDepsByValue,
			  std::map<std::string, const Type*>* allDepsByName);
  int getNumField(GetElementPtrInst* inst);

};

#endif
