#include <iostream>

#include "globals.h"
#include "server/accumserver.h"
#include "exception/accumexception.h"

int main()
{
	std::cout << "Version: " << VERSION << std::endl;

	AccumServer server;
	server.setIpAddress("192.168.0.221");
	server.setPort(1028);
	server.setMaxClientsNum(1);

	try
	{
		server.openServer();
	}
	catch(AccumException &e)
	{
		std::cout << e.what() << std::endl;
	}

	server.closeServer();
	
	return 0;
}

