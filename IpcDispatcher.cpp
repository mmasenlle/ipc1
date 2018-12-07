
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "logger.h"
#include "box_ipc/box_ipc.h"
#include "IpcDispatcher.h"


IpcDispatcher::IpcDispatcher() : fd(-1) { }

IpcDispatcher::~IpcDispatcher()
{
	if(fd != -1) close(fd);
}

IpcDispatcher::IpcDispatcher(unsigned id)
{
	plug(id);
}

void IpcDispatcher::plug(unsigned id)
{
	fd = open("/dev/box_ipc", O_RDWR | O_NDELAY);
	if(fd == -1)
	{
		SELOG("IpcDispatcher::IpcDispatcher -> openning '/dev/box_ipc' ");
	}
	else
	{
		box_ipc_msg_t bim;
		bim.from = id;
		bim.type = BOXIPC_MSG_TYPE_REGISTER;
		if(write(fd, &bim, sizeof(bim)) == -1)
		{
			SELOG("IpcDispatcher::IpcDispatcher -> registering (%x) ", id);
			close(fd); fd = -1;
			fprintf(stderr, "Other instance of task (%x) may be running : ",id); perror("");
			_exit(-1);
		}
		int flags = fcntl(fd, F_GETFD);
		if((flags == -1) || (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1))
		{
			SELOG("IpcDispatcher::IpcDispatcher -> setting FD_CLOEXEC (%x) ", id);
		}
	}
}

int IpcDispatcher::dispatch()
{
	char *dbuf = NULL, sbuf[2048];
	int r = read(fd, sbuf, sizeof(sbuf));
	if (r == 0) return 0;
	if (r < 0) {
		SELOG("IpcDispatcher::dispatch -> reading ");
		return (-1);
	}
	if (r > (int)sizeof(sbuf)) {
		dbuf = new char[r];
		if ((r = read(fd, dbuf, r)) <= 0) {
			SELOG("IpcDispatcher::dispatch -> reading 2 ");
			delete dbuf;
			return (-1);
		}
	}
	box_ipc_msg_t *bim = (box_ipc_msg_t *)(dbuf ? : sbuf);
	if (bim->type == BOXIPC_MSG_TYPE_XML) {
		Message *m = Message::create(bim->msg, r - sizeof(box_ipc_msg_t));
		if (m) {
			r = (process(m, bim->from) + 1);
			delete m;
		} else {
			ELOG("IpcDispatcher::dispatch -> unrecognized XML message from: %lx", bim->from);
			r = -1;
		}
	} else {
		WLOG("IpcDispatcher::dispatch -> dispatcher for msg type %d not implemented", (int)bim->type);
		r = -1;
	}

	delete dbuf;
	return r;
}

int IpcDispatcher::send(unsigned to, const Message &msg)
{
	std::string sbuf;
	msg.serialize(&sbuf);
	int len = sizeof(box_ipc_msg_t) + sbuf.length();
	box_ipc_msg_t *bim = (box_ipc_msg_t *)new char[len];
	bim->from = to;
	bim->type = BOXIPC_MSG_TYPE_XML;
	memcpy(bim->msg, sbuf.c_str(), sbuf.length());
	int r = write(fd, bim, len);
	delete [] (char*)bim;
	if(r == -1)
	{
		SELOG("IpcDispatcher::send -> sending to %x", to);
	}
	else if(r == 0)
	{
		WLOG("IpcDispatcher::send -> addressee (%x) not present", to);
	}
	return r;
}

