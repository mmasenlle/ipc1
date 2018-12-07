
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "IpcManager.h"

main(int argc, char *argv[])
{
	if(argc < 2 || atoi(argv[1]) < 1) exit(-1);
	IpcManager worker(atoi(argv[1]), "/pruebas_ipc");
	printf("Initialization done!! (%d)\n\n", atoi(argv[1]));
	int i = 0;
	while(1)
	{
		char buff[256]; int n, from;
		printf("(%d) worker.read ...\n", atoi(argv[1]));
		while((n = worker.read(buff, sizeof(buff), &from, 30000)) > 0)
		{
			printf("(%d) From %d:\t", atoi(argv[1]), from);
			write(1, buff, n); printf("\n");
		}
//		n = snprintf(buff, sizeof(buff), "hello i am a worker (%d)", i++);
//		worker.write(atoi(argv[1]) + 1, buff, n);
//		sleep(2);
	}
}
