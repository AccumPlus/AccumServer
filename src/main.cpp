#include <iostream>
#include <json.hpp>
#include <signal.h>
#include <stdlib.h>

#include "globals.h"
#include "server/accumserver.h"
#include "exception/accumexception.h"

void catchCtrlC(int s);

AccumServer server;

int main()
{
	std::cout << "=== AccumServer ===" << std::endl << "Version: " << VERSION << std::endl;

	/* Обработка Ctrl+C сигнала */
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = catchCtrlC;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	try
	{
		server.init("accumserver.json");
		server.openServer();
	}
	catch(AccumException &e)
	{
		std::cout << e.what() << std::endl;
	}

	/* По-хорошему, здесь сервер уже закрыт */
	server.closeServer();
	
	return 0;
}

void catchCtrlC(int s)
{
	server.closeServer();
}
