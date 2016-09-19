#include <iostream>
#include <json.hpp>

#include "globals.h"
#include "server/accumserver.h"
#include "exception/accumexception.h"

int main()
{
	std::cout << "=== AccumServer ===" << std::endl << "Version: " << VERSION << std::endl;

	AccumServer server;

	try
	{
		server.init("accumserver.json");
		server.openServer();
	}
	catch(AccumException &e)
	{
		std::cout << e.what() << std::endl;
	}

	server.closeServer();
	
	return 0;
}

