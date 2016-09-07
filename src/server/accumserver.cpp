#include <iostream>
#include <regex>
#include <errno.h>

#include <globals.h>

#include "accumserver.h"

AccumServer::AccumServer():
	maxClientsNum{0}
{
}

AccumServer::~AccumServer()
{
}

void AccumServer::setIpAddress(const std::string &ipAddress)
{
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
		throw AccumException(AccumException::SRV_CREAT_SOCK_EXC);

	srvAddr.sin_family = AF_INET;
	srvAddr.sin_port = htons(port);
	inet_aton (ipAddress.c_str(), &srvAddr.sin_addr);

	if (bind(sockDescr, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0)
	{
		close (sockDescr);
		switch (errno)
		{
		case EADDRINUSE:
			throw AccumException(AccumException::SRV_BIND_PORT_EXC);
		case EADDRNOTAVAIL:
			throw AccumException(AccumException::SRV_BIND_IP_EXC);
		default:
			throw AccumException(AccumException::DEFAULT_EXC);
		}
	}

	if (listen (sockDescr, maxClientsNum) < 0)
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
