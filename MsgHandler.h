#ifndef MSGHANDLER_H_
#define MSGHANDLER_H_

#include <map>
#include "Message.h"

class MsgHandler
{
protected:
	std::map<unsigned long, MsgHandler *> handlers;
	int processDefault(const Message *msg, unsigned long from);
	virtual int process(const Message *msg, unsigned long from);
public:
	void addHandler(unsigned long taskid, MsgHandler *handler);
};


#define DEC_HANDLER(id) \
	case id: return processM##id (msg, from);

#endif /*MSGHANDLER_H_*/
