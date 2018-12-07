
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "IpcManager.h"

class ipc_mng : public IManager
{
	void killall()
	{
		//system("killall -9 ipc_worker >/dev/null 2>&1");
	};
	void start(int id)
	{
		char cmd[128];
		snprintf(cmd, sizeof(cmd), "./ipc_worker %d &", id);
		system(cmd);
	};
};

main()
{
	ipc_mng mng;
	mng.init("/pruebas_ipc");
	printf("Initialization done!! (manager)\n\n");
	while(1/*getchar() != 'q'*/)
	{
		sleep(5);
		mng.check();
	}
}
