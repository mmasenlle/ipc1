#ifndef BOX_IPC_H_
#define BOX_IPC_H_

enum
{
	BOXIPC_MSG_TYPE_REGISTER,
	BOXIPC_MSG_TYPE_RAW,
	BOXIPC_MSG_TYPE_XML
};

#define BOXIPC_ID_BROADCAST	0xFFFFFFFF
#define BOXIPC_GROUP_MASK	0x00FF0000

struct box_ipc_msg_t
{
	unsigned long from;
	unsigned long type;
	char msg[0];
};

#endif /*BOX_IPC_H_*/
