#include <iostream>
#include <regex>
#include <errno.h>

#include <globals.h>

#include "accumserver.h"

AccumServer::AccumServer():
	maxClientsNum{1}, curClientsNum{0}
{
}

AccumServer::AccumServer(const std::string &ipAddress, const short &port, const int num):
	ipAddress(ipAddress), port(port), maxClientsNum(num)
{
}

AccumServer::~AccumServer()
{
	closeServer();
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

void AccumServer::openServer() throw (AccumException)
{
	std::regex pattern("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
	if (!std::regex_match(ipAddress, pattern))
		throw AccumException(AccumException::INV_IP_STR_EXC);
	if (maxClientsNum < 0)
		throw AccumException(AccumException::INV_CL_NUM_EXC);

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
		throw AccumException(AccumException::SRV_LIST_EXC);
	}

	opened = true;

	while (opened)
	{
		struct sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);
		int clientSocket = accept(sockDescr, (struct sockaddr*)&clientAddr, (socklen_t*)&addrLen);
		if (clientSocket < 0)
		{
			close(clientSocket);
			if (opened)
				throw AccumException(AccumException::SRV_ACCEPT_EXC);
		}
		else
		{
			std::cout << "New connection:" << std::endl;
			std::cout << "IP: " << inet_ntoa(clientAddr.sin_addr) << std::endl;

			// Ждём, если все заняты
			while (curClientsNum >= maxClientsNum);

			++curClientsNum;

			Client client;
			client.clientSocket = clientSocket;
			client.address = inet_ntoa(clientAddr.sin_addr);
			client.th = std::thread(&AccumServer::readData, this, clientSocket);

			clients.push_back(std::move(client));
		}
	}
}

void AccumServer::closeServer()
{
	if (opened)
	{
		close(sockDescr);
		opened = false;
	}
}

bool AccumServer::isOpened()
{
	return opened;
}

void AccumServer::readData(int clientSocket)
{
	char message[BUFSIZ];
	int bytesNumber;

	while (true)
	{
		bytesNumber = recv(clientSocket, message, BUFSIZ, 0);
		if (bytesNumber <= 0) break;

		std::cout << clients[findClient(clientSocket)].address << std::endl << message << std::endl << std::endl;
		break;
	}

	removeClient(clientSocket);
}

int AccumServer::findClient(int clientSocket)
{
	int n = 0;
	for (auto it = clients.begin(); it != clients.end(); ++it, ++n)
		if (it->clientSocket == clientSocket)
			return n;
	return -1;
}

void AccumServer::removeClient(int clientSocket)
{
	int n = findClient(clientSocket);
	if (n == -1)
		return;

	close(clientSocket);
	clients[n].th.detach();
	clients.erase(clients.begin() + n);

	--curClientsNum;
}
