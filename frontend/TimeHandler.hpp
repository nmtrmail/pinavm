#ifndef _TIMEHANDLER_HPP
#define _TIMEHANDLER_HPP

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Function.h"
#include "llvm/Instructions.h"

#include "SCConstructHandler.hpp"

using namespace llvm;
struct SCJit;

class TimeHandler:public SCConstructHandler {
      protected:
	int time;

      public:
	 TimeHandler(SCJit * jit):SCConstructHandler(jit) {}
	int getTime();
};

#endif
