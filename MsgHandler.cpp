
#include "globals.h"
#include "logger.h"
#include "MsgHandler.h"

int MsgHandler::processDefault(const Message *msg, unsigned long from)
{
	if(handlers.find(from & TASKID_INSTANCE_MASK) != handlers.end())
	{
		return handlers[from & TASKID_INSTANCE_MASK]->process(msg, from);
	}
	if(handlers.find(from & TASKID_TASK_MASK) != handlers.end())
	{
		return handlers[from & TASKID_TASK_MASK]->process(msg, from);
	}
	if(handlers.find(from & TASKID_TYPE_MASK) != handlers.end())
	{
		return handlers[from & TASKID_TYPE_MASK]->process(msg, from);
	}
	if(handlers.find(TASKID_DEFAULT_H) != handlers.end() && (handlers[TASKID_DEFAULT_H] != this))
	{
		return handlers[TASKID_DEFAULT_H]->process(msg, from);
	}
	WLOG("MsgHandler::processDefault -> Not handler found for msg (%d) from (%lx)", msg->getId(), from);
	return (-1);
}

void MsgHandler::addHandler(unsigned long taskid, MsgHandler *handler)
{
	handlers[taskid] = handler;
}

int MsgHandler::process(const Message *msg, unsigned long from)
{
	return processDefault(msg, from);
}

