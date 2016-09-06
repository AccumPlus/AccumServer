#include <iostream>
#include <regex>

#include <globals.h>

#include "server.h"


void Server::setIpAddress(const std::string &ipAddress)
{
	/*std::regex pattern("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
	if (!std::regex_match(ipAddress, pattern))
		return INV_IP_ERR;*/
	this->ipAddress = ipAddress;
}

void Server::setPort(const short &port)
{
	this->port = port;
}

void Server::setMaxClientsNum(const int &num)
{
	this->maxClientsNum = num;
}

std::string Server::getIpAddress()
{
	return ipAddress;
}

short Server::getPort()
{
	return port;
}

int Server::getMaxClientsNum()
{
	return maxClientsNum;
}

/*int open()
{
	sockDescr = socket(AF_INET, SOCK_STREAM, 0);
	if (sockDescr < 0)
		return SRV_CREATE_ERR;

	return NO_ERR;
}*/
