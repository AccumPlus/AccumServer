#ifndef ACCUMSERVER_H
#define ACCUMSERVER_H

#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <sys/types.h>

#include <exception/accumexception.h>
#include <json.hpp>

class AccumServer
{
	public:
		AccumServer();
		~AccumServer();

		void init(const std::string &settingsFile) throw (AccumException);
		void catchCtrlC();

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
		void watchDog(int clientSocket, pid_t pid, bool &stopWatchDog);

		std::string ipAddress;				// IP адрес сервера
		short port;							// Порт сервера
		int maxClientsNum;					// Максимальное число поделючений
		std::vector<std::string> whitelist;	// "Белый" список IP адресов (подключаться можно только им)
		std::vector<std::string> blacklist;	// "Чёрный" список IP адресов (им подключаться нельзя)
		char *program;						// Программа-обработчик запросов
		char **args;						// Аргументы для программы-обработчика
		std::string pipePath;				// Путь к каталогу с трубами

		int sockDescr;						// Дескриптор сервер-сокета
		sockaddr_in srvAddr;				// Структура для хранения адреса сервера
		bool opened;						// Флаг открыт ли сервер
		int	curClientsNum;					// Текущее число подключений
		bool closing;

		struct Client
		{
			Client():
				clientSocket(0)
			{}
			int clientSocket;
			std::string address;
			std::thread th;
		};
		std::mutex mutexClients;
		std::mutex mutexWatchDog;
		std::mutex mutexClosing;
		std::vector<Client> clients;
};

#endif
