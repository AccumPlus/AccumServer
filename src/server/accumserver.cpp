#include <iostream>
#include <regex>

//#include <globals.h>

#include "accumserver.h"

AccumServer::AccumServer()
{
}

AccumServer::~AccumServer()
{
}

void AccumServer::setIpAddress(const std::string &ipAddress)
{
	/*std::regex pattern("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
	if (!std::regex_match(ipAddress, pattern))
		return INV_IP_ERR;*/
	this->ipAddress = ipAddress;
}

void AccumServer::setPort(const short &port)
{
	this->port = port;
}

void AccumServer::setMaxClientsNum(const int &num)
{
	this->maxClientsNum = num;
}

std::string AccumServer::getIpAddress()
{
	return ipAddress;
}

short AccumServer::getPort()
{
	return port;
}

int AccumServer::getMaxClientsNum()
{
	return maxClientsNum;
}

int AccumServer::openServer() throw (AccumException)
{
	sockDescr = socket(AF_INET, SOCK_STREAM, 0);
	if (sockDescr < 0)
		return -1;

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(port);
	inet_aton (ipAddress.c_str(), &srvAddr.sin_addr);

	if (bind(sockDescr, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
	{
		close (sockDescr);
		throw AccumException(AccumException::SRV_BIND_EXC);
	}

	if (listen (sockDescr, 0) < 0)
	{
		close (sockDescr);
		return -3;
	}

	return 0;
}

void AccumServer::closeServer()
{
	close(sockDescr);
}
