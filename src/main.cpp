#include <iostream>

#include "globals.h"
#include "server/accumserver.h"
#include "server/accumexception.h"

int main()
{
	std::cout << "Version: " << VERSION << std::endl;

	AccumServer server;
	server.setIpAddress("AAAAA");
	server.setPort(1028);
	int result;
	try
	{
		result = server.openServer();
	}
	catch(AccumException &e)
	{
		std::cout << e.what() << std::endl;
	}
	server.closeServer();
	
	return 0;
}

