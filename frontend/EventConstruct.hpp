#ifndef _EVENTCONSTRUCT_HPP
#define _EVENTCONSTRUCT_HPP

#include "llvm/Function.h"
#include "llvm/Instructions.h"

#include "SCConstruct.hpp"
#include "Event.hpp"

using namespace llvm;
using namespace std;

struct EventConstruct : public SCConstruct {  
protected :
  Event* event;
public:
  EventConstruct(Event* e);
  void setWaitedEvent(Event* e);
  Event* getWaitedEvent();
  string toString();
};

#endif