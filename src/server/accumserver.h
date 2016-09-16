#ifndef ACCUMSERVER_H
#define ACCUMSERVER_H

#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>

#include <exception/accumexception.h>

class AccumServer
{
	public:
		AccumServer();
		AccumServer(const std::string &ipAddress, const short &port, const int num = 0);
		~AccumServer();

		void setIpAddress(const std::string &ipAddress);
		void setPort(const short &port);
		void setMaxClientsNum(const int &num);

		std::string getIpAddress();
		short getPort();
		int getMaxClientsNum();
		bool isOpened();

		void openServer() throw (AccumException);
		void closeServer();

		std::string getRequest();

	private:
		void readData(int clientSocket);
		int findClient(int clientSocket);
		void removeClient(int clientSocket);

		std::string ipAddress;				// IP адрес сервера
		short port;							// Порт сервера
		int maxClientsNum;					// Максимальное число поделючений

		int sockDescr;						// Дескриптор сервер-сокета
		sockaddr_in srvAddr;				// Структура для хранения адреса сервера
		bool opened;						// Флаг открыт ли сервер
		int	curClientsNum;					// Текущее число подключений

		struct Client
		{
			Client():
				clientSocket(0)
			{}
			int clientSocket;
			std::string address;
			std::thread th;
		};
		std::vector<Client> clients;

		
};

#endif
