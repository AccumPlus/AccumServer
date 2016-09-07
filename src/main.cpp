#include <iostream>

#include "server/accumserver.h"
#include "globals.h"

int main()
{
	std::cout << "Version: " << VERSION << std::endl;

	AccumServer server;
	server.setIpAddress("127.0.0.1");
	server.setPort(1028);
	int result;
	result = server.openServer();
	dprint(result);
	server.closeServer();

	return 0;
}

