#include <iostream>
#include <class/class.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG
	#define dprint(x) do {std::cerr << x << std::endl;}while(0)
#else
	#define dprint(x) do {}while(0)
#endif

int main()
{
	std::cout << "Version: " << VERSION << std::endl;

	int sd;
	struct sockaddr_in addr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
	{
		dprint("On create server\n");
		return 1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(1028);
	inet_aton("0.0.0.0", &addr.sin_addr);

	if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		close(sd);
		dprint("On bind server\n");
		perror("bind");
		return 1;
	}

	if (listen(sd, 1) < 0)
	{
		close(sd);
		dprint("On listen server\n");
		return 1;
	}

	while (1)
	{
		struct sockaddr_in addrc;
		socklen_t addrLen = sizeof(addrc);

		int sdc = accept(sd, (struct sockaddr*)&addrc, &addrLen);
		if (sdc < 0)
		{
			dprint("On create client\n");
			break;
		}

		char message[BUFSIZ];
		int bytesNumber = recv(sdc, message, BUFSIZ, 0);
		if (bytesNumber <= 0)
			break;
		
		std::cout << "Mes = " << message << std::endl;

	}

	
	return 0;
}

