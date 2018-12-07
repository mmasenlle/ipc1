#ifndef IPCDISPATCHER_H_
#define IPCDISPATCHER_H_

#include "MsgHandler.h"

class IpcDispatcher : public MsgHandler
{
	int fd;
public:
	IpcDispatcher();
	~IpcDispatcher();
	IpcDispatcher(unsigned id);
	void plug(unsigned id);
	int get_fd() { return fd; };
	int dispatch();
	int send(unsigned to, const Message &msg);
};

#endif /*IPCDISPATCHER_H_*/
