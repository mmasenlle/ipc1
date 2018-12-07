
IPCFLAGS = IpcManager.o -pthread -lrt
CXXFLAGS = -g -I..

all: manager worker worker_w

manager: IpcManager.o ipc_manager.o
	g++ -o ipc_manager ipc_manager.o $(IPCFLAGS)

worker: IpcManager.o ipc_worker.o
	g++ -o ipc_worker ipc_worker.o $(IPCFLAGS)
	
worker_w: IpcManager.o ipc_worker_w.o
	g++ -o ipc_worker_w ipc_worker_w.o $(IPCFLAGS)
	
IpcManager.o: ../IpcManager.cpp ../IpcManager.h
	g++ -c ../IpcManager.cpp

clean:
	rm -f ipc_manager ipc_worker *.o
