#ifndef _CHANNEL_HPP
#define _CHANNEL_HPP

#include <string>
#include <vector>
#include <llvm/IR/Type.h>

class Port;

typedef enum {
	UNDEFINED_CHANNEL,
	SIMPLE_CHANNEL,
	FORWARDING_CHANNEL,
	FIFO_CHANNEL,
	CLOCK_CHANNEL,
	BASIC_CHANNEL
} channel_id;

class Channel {
protected:
	channel_id id;
	std::vector<Port*>* ports;
	llvm::Type* type;
	std::string chTypeName;

public:
	Channel(llvm::Type* t, std::string typeName);
	~Channel();
	std::vector<Port*>* getPorts();
	channel_id getID() const;
	llvm::Type* getType();
	std::string getTypeName();
	void addPort(Port* port);
	virtual std::string toString() = 0;
};


#endif
