
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include "box_ipc.h"

static int sbuf[30 * 1024];

int main(int argc, char *argv[])
{
	int r, id, i = 0, j;
	struct pollfd pfd;
	struct box_ipc_msg_t sbim;
	char buffer[1024];
	if(argc < 2)
	{
		fprintf(stderr, "usage: %s <id>\n\n", argv[0]);
		return -1;
	}
	id = atoi(argv[1]);
	pfd.events = POLLIN;
	pfd.fd = open("/dev/box_ipc", O_RDWR);
	if(pfd.fd == -1)
	{
		perror("Openning /dev/box_ipc ");
		return -1;
	}
	sbim.from = id;
	sbim.type = BOXIPC_MSG_TYPE_REGISTER;
	if(write(pfd.fd, &sbim, sizeof(sbim)) != sizeof(sbim))
	{
		perror("registering ");
		return -1;
	}
	
	for(;;)
	{
		system("more /proc/box_ipc");
		printf("\n\n<r> leer\n<w> escribir\n<f> mandar fichero\n<m> prueba de memoria\n<q> salir\n");
		printf("Introduzca comando: ");
ign:	switch(getchar())
		{
		case 'r':
			if(poll(&pfd, 1, 25000) == -1)
			{
				perror("Error in poll");
			}
			else if(pfd.revents != POLLIN)
			{
				printf("Timeout or something (%d)\n", pfd.revents);
				continue;
			}
			r = read(pfd.fd, buffer, sizeof(buffer));
			printf("\nReturn: %d\n", r);
			if(r < 0) perror("Reading ");
			else if(r >= sizeof(struct box_ipc_msg_t) && r <= sizeof(buffer))
			{
				struct box_ipc_msg_t *bim = (struct box_ipc_msg_t *)buffer;
				printf("From: %d, type: %d, len: %d\n", bim->from, bim->type, r);
				write(1, bim->msg, r - sizeof(struct box_ipc_msg_t));
			}
			else if(r > sizeof(buffer))
			{
				struct box_ipc_msg_t *bim = (struct box_ipc_msg_t *)malloc(r);
				int r2 = read(pfd.fd, bim, r);
				if(r2 <= 0) perror("Reading ");
				else
				{
					write(1, bim->msg, r - sizeof(struct box_ipc_msg_t));
					printf("\n\nFrom: %d, type: %d, len: %d/%d\n", bim->from, bim->type, r2, r);
				}
				free(bim);
			}
			break;
		case 'w':
			printf("\nIntroduzca destino: ");
			r = 0; scanf("%d", &r);
			if(r > 0)
			{
				struct box_ipc_msg_t *bim = (struct box_ipc_msg_t *)buffer;
				bim->from = r > 100 ? BOXIPC_ID_BROADCAST : r;
				bim->type = BOXIPC_MSG_TYPE_XML;
				r = sprintf(bim->msg, "Mensaje de %d (%d), saludos **** \n", id, i++);
				if((r = write(pfd.fd, bim, r + sizeof(struct box_ipc_msg_t))) < 0)
				{
					perror("Writting ");
				}
				printf("Enviado a %d (%d)\n", bim->from, r);
			}
			break;
		case 'f':
			printf("\nIntroduzca destino: ");
			r = 0; scanf("%d", &r);
			if(r > 0)
			{
				int f;
				printf("\nIntroduzca path del fichero: "); scanf("%s", buffer);
				if((f = open(buffer, O_RDONLY)) == -1)
				{
					fprintf(stderr, "Error opening %s ", buffer); perror("");
				}
				else
				{
					struct box_ipc_msg_t *bim;
					struct stat st;
					fstat(f,&st);
					printf("Reading %s (%d bytes) ...\n", buffer, st.st_size);
					bim = (struct box_ipc_msg_t *)malloc(st.st_size + sizeof(struct box_ipc_msg_t));
					if(!bim) fprintf(stderr, "Error allocating memory\n");
					else
					{
						int n, n2 = 0;
						bim->from = r > 100 ? BOXIPC_ID_BROADCAST : r;
						bim->type = BOXIPC_MSG_TYPE_RAW;		
						while((n = read(f, bim->msg + n2, st.st_size - n2)) > 0) n2 += n;
						if((n = write(pfd.fd, bim, st.st_size + sizeof(struct box_ipc_msg_t))) < 0)
						{
							perror("Writting ");
						}
						printf("Enviado a %d (%d)\n", bim->from, n);
						free(bim);
					}
					close(f);
				}
			}
			break;
		case 'm':
			if(!sbuf[0])
			{
				for(j = 0; j < sizeof(sbuf)/sizeof(sbuf[0]); j++) sbuf[j] = 'aloh';
				struct box_ipc_msg_t *bim = (struct box_ipc_msg_t *)sbuf;
				bim->from = BOXIPC_ID_BROADCAST;
				bim->type = BOXIPC_MSG_TYPE_RAW;
			}
			while(write(pfd.fd, sbuf, sizeof(sbuf)) > 0);
			perror("Durante prueba de memoria ");
			break;
		case '\n': goto ign;
		case 'q':
			return 0;
		}
	}
	
	return 0;
}

