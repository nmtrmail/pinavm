#ifndef _SIMPLECHANNEL_HPP
#define _SIMPLECHANNEL_HPP

#include <string>

#include "Channel.hpp"

#include <llvm/IR/Type.h>


class SimpleChannel : public Channel
{
protected:
	std::string globalVariableName;
public:
	SimpleChannel(llvm::Type* globalVariableType, std::string globalVarTypeName);
	std::string getGlobalVariableName();
	std::string toString();
	static inline bool classof(const Channel *ch) {
		return (ch->getID() == SIMPLE_CHANNEL);
	}
};

#endif
