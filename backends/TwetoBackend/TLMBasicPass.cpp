/**
 * TLMBasicPass.cpp
 *
 * 
 *
 * @author : Si-Mohamed Lamraoui
 * @contact : si-mohamed.lamraoui@imag.fr
 * @date : 2011/03/17
 * @copyright : Verimag 2011
 */

#include <string>
#include <map>

#include "llvm/GlobalValue.h"
#include "llvm/CallingConv.h"
#include "llvm/DerivedTypes.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/LLVMContext.h"
#include "llvm/Analysis/Verifier.h"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include "Port.hpp"
#include "Channel.hpp"
#include "SimpleChannel.hpp"
#include "SCCFactory.hpp"
#include "SCElab.h"
#include "SCConstruct.hpp"
#include "EventConstruct.hpp"
#include "ReadConstruct.hpp"
#include "WriteConstruct.hpp"
#include "AssertConstruct.hpp"
#include "RandConstruct.hpp"
#include "Process.hpp"
#include "IRModule.hpp"
#include "SCJit.hpp"

#include "sysc/kernel/sc_process_table.h"
#include "sysc/kernel/sc_simcontext.h"
#include "sysc/kernel/sc_thread_process.h"
#include "sysc/kernel/sc_module_registry.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_process.h"
#include "sysc/communication/sc_port.h"

#include "basic.h"
#include "bus.h"

#include "TLMBasicPass.h"


char TLMBasicPass::ID = 0;
const std::string wFunName = "_ZN5basic21initiator_socket_baseILb0EE5writeERKjji";
const std::string rFunName = "_ZN5basic21initiator_socket_baseXXXXXXXXXXXXXXXXXX";


// =============================================================================
// TLMBasicPass 
// 
// Constructor
// 
// =============================================================================
TLMBasicPass::TLMBasicPass(Frontend *fe) : ModulePass(ID) {
    this->callOptCounter = 0;
    this->rwCallsCounter = 0;
    this->fe = fe; 
    this->elab = fe->getElab();
    // Initialize function passes
    Module *mod = this->fe->getLLVMModule();
    TargetData *target = new TargetData(mod);
    funPassManager = new FunctionPassManager(mod);
    funPassManager->add(target);
    funPassManager->add(createIndVarSimplifyPass());
    funPassManager->add(createLoopUnrollPass());
    funPassManager->add(createInstructionCombiningPass());
    funPassManager->add(createReassociatePass());
    funPassManager->add(createGVNPass());
    funPassManager->add(createCFGSimplificationPass());
    funPassManager->add(createConstantPropagationPass());
}


// =============================================================================
// runOnModule
// 
//  
// 
// =============================================================================
bool TLMBasicPass::runOnModule(Module &M) {
    
    std::cout << "\n============== TLM Basic Pass =============\n";

    // Modules
    vector < sc_core::sc_module * >modules =
    sc_core::sc_get_curr_simcontext()->get_module_registry()->m_module_vec;
	vector < sc_core::sc_module * >::iterator modIt;
	for (modIt = modules.begin(); modIt < modules.end(); ++modIt) {
		sc_core::sc_module * initiatorMod = *modIt;
        std::string moduleName = (std::string) initiatorMod->name();
        
        // Initiator ports
		std::vector < sc_core::sc_port_base * >*ports = initiatorMod->m_port_vec;
		vector < sc_core::sc_port_base * >::iterator it;
		for (it = ports->begin(); it < ports->end(); ++it) {
			sc_core::sc_port_base * initiator = *it;
            sc_core::sc_interface* initiatorItf = initiator->get_interface();
        
            // DO NOT WORK 
            //const char* initiatorTypeName = typeid(*initiatorItf).name();            
            //std::string initiatorItfTypeName(initiatorTypeName);
            //std::string match1 = "N5basic21initiator_socket_trueE";
            //std::string match2 = "N5basic16initiator_socket";
            //if ((initiatorItfTypeName.find(match1) == 0) 
            //  || (initiatorItfTypeName.find(match2) == 0)) {
            
            std::string initiatorName = initiator->name(); 
            if (initiatorName.find("basic::initiator_socket")!=string::npos) {
                
                std::cout << "== Initiator : " << initiatorName << std::endl;
               
                // Target ports
                basic::target_socket_base<true> *tsb =
                dynamic_cast<basic::target_socket_base<true> *>(initiatorItf);
                if (tsb) {
                    Bus *b = dynamic_cast<Bus *>(tsb->get_parent_object());
                    for(int i = 0; i < b->initiator.size(); ++i) {
                        basic::compatible_socket* target =
                        dynamic_cast<basic::compatible_socket*>(b->initiator[i]);
                        if (target) {
                            std::cout << " = Target : " << target->name() 
                            << std::endl; 
                            
                            sc_core::sc_object *o = target->get_parent();
                            sc_core::sc_module *targetMod = 
                            dynamic_cast<sc_core::sc_module *>(o);

                            optimize(target, initiatorMod, targetMod, b); 
                            
                        }
                    }
                
                }                
                
            }
		}
	}
    std::cout << "\n Pass report - " << callOptCounter 
    << "/" << "?" << " - opt/total" << std::endl;
    
    std::cout << "===========================================\n\n";
}


// =============================================================================
// optimize
// 
// Optimize all process that use write or read functions
// =============================================================================
void TLMBasicPass::optimize(basic::compatible_socket* target, 
                            sc_core::sc_module *initiatorMod, 
                            sc_core::sc_module *targetMod, 
                            Bus *bus) {

    // Looking for declarations of functions
    Function *writef = lookForWriteFunction(targetMod);
    Function *readf = lookForReadFunction(targetMod);
    
    // Looking for calls in process
    sc_core::sc_process_table * processes = 
    initiatorMod->sc_get_curr_simcontext()->m_process_table;
    sc_core::sc_thread_handle thread_p;
    for (thread_p = processes->thread_q_head(); thread_p; thread_p = thread_p->next_exist()) {
        sc_core::sc_process_b *proc = thread_p;
        std::cout << "      Replace in thread : " << proc << std::endl;
        replaceCallsInProcess(target, initiatorMod, proc, writef, readf, bus);
    }
    sc_core::sc_method_handle method_p;
    for (method_p = processes->method_q_head(); method_p; method_p = method_p->next_exist()) {
        //sc_core::sc_method_process *proc = method_p;
        sc_core::sc_process_b *proc = method_p;
        std::cout << "      Replace in method : " << proc << std::endl;
        replaceCallsInProcess(target, initiatorMod, proc, writef, readf, bus);
    }
    
}


// =============================================================================
// replaceCallsInProcess
// 
// Replace indirect calls to write() or read() by direct calls 
// in the given process.
// =============================================================================
void TLMBasicPass::replaceCallsInProcess(basic::compatible_socket* target, 
                                         sc_core::sc_module *mod, 
                                         sc_core::sc_process_b *proc,
                                         Function *writef, Function *readf, 
                                         Bus *bus) {
   
    // Get associate function
    std::string fctName = proc->func_process;
	std::string modType = typeid(*mod).name();
	std::string mainFctName = "_ZN" + modType + utostr(fctName.size()) + fctName + "Ev";
	Function *procf = this->fe->getLLVMModule()->getFunction(mainFctName);
    
    // Run preloaded passes on the function to propagate constants
    //funPassManager->run(*procf);
    //verifyFunction(*procf);
    
    
    std::map<CallSite,Instruction*> callsToReplace;
    
    inst_iterator ii;
    for (ii = inst_begin(procf); ii!=inst_end(procf); ii++) {
        Instruction &i = *ii;
        CallSite cs = CallSite::get(&i);
        if (cs.getInstruction()) {
            // Candidate for a replacement
            Function *oldfun = cs.getCalledFunction();
            if (oldfun!=NULL && !oldfun->isDeclaration()) {
                std::string name = oldfun->getName();
                
                // === Write ===
                if (writef!=NULL && !strcmp(name.c_str(), wFunName.c_str())) {
                    std::cout << "       Checking adress : ";
                    // Retreive the argument by executing 
                    // the appropriated piece of code
                    SCJit *scjit = new SCJit(this->fe->getLLVMModule(), this->elab);
                    Process *irProc = this->elab->getProcess(proc);
                    scjit->setCurrentProcess(irProc);
                    bool errb = false;
                    Value *arg = cs.getArgument(1);
                    Instruction *inst = cs.getInstruction();
                    int value = scjit->jitInt(procf, inst, arg, &errb);
                    std::cout << "0x" << std::hex << value << std::endl;
                    basic::addr_t a = static_cast<basic::addr_t>(value);
                    
                    // Checking adress and target concordance
                    bool concordErr = bus->checkAdressConcordance(target, a);
                    if(!concordErr) {
                        std::cout << "       return, no concordances!" << std::endl;
                        return;
                    }
                    
                    // Checking adress alignment
                    if(value % sizeof(basic::data_t)) {
                        std::cerr << "  unaligned write : " <<
                        std::hex << value << std::endl;
                        abort();
                    }
                    
                    // Checking address range
                    bool addrErr = bus->checkAdressRange(a);
                    if(!addrErr) {
                        std::cerr << "  no target at address " <<
                        std::hex << value << std::endl;
                        abort();
                    }
                     
                    // Resolve the undirect call
                    cs.setCalledFunction(writef);
                    callOptCounter++;
                    std::cout << "       Call optimized (^_^)" << std::endl;
                    
                } else 
                    
                // === Read ===
                if (readf!=NULL && !strcmp(name.c_str(), rFunName.c_str())) {
                    
                    // Not yet supported
                                        
                }
            }  
        }
    }
    
}


// =============================================================================
// lookForWriteFunction
// 
// Look for a write function definition 
// in the given SystemC module
// =============================================================================
Function* TLMBasicPass::lookForWriteFunction(sc_core::sc_module *module) {
    Module *llvmMod = this->fe->getLLVMModule();
    // Reverse mangling
    std::string moduleType = typeid(*module).name();    
    std::string name("_ZN"+moduleType+"5writeERKjS1_");
    Function *f = llvmMod->getFunction(name);
    if(f!=NULL) {
        std::string moduleName = (std::string) module->name();
        std::cout << "      Found 'write' function in the module : " 
        << moduleName << std::endl;
    }
    return f;
}


// =============================================================================
// lookForWriteFunction
// 
// Look for a read function definition 
// in the given SystemC module
// =============================================================================
Function* TLMBasicPass::lookForReadFunction(sc_core::sc_module *module) {
    Module *llvmMod = this->fe->getLLVMModule();
    // Reverse mangling
    std::string moduleType = typeid(*module).name();
    std::string name("_ZN"+moduleType+"4readERKjRj");    
    Function *f = llvmMod->getFunction(name);
    if(f!=NULL) {
        std::string moduleName = (std::string) module->name();
        std::cout << "      Found 'read' function in the module : " 
        << moduleName << std::endl;
    }
    return f;
}
