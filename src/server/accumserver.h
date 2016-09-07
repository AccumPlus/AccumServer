#ifndef ACCUMSERVER_H
#define ACCUMSERVER_H

#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "accumexception.h"

class AccumServer
{
	public:
		AccumServer();
		~AccumServer();

		void setIpAddress(const std::string &ipAddress);
		void setPort(const short &port);
		void setMaxClientsNum(const int &num);

		std::string getIpAddress();
		short getPort();
		int getMaxClientsNum();

		int openServer() throw (AccumException);
		void closeServer();

	private:
		std::string ipAddress;
		short port;
		int maxClientsNum;

		int sockDescr;
		sockaddr_in srvAddr;
};

#endif
