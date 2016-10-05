#include <iostream>
#include <regex>
#include <errno.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>

#include <globals.h>

#include "accumserver.h"

AccumServer::AccumServer():
	maxClientsNum{1}, curClientsNum{0}, ipAddress{""}, port{0}, sockDescr{0}, opened{false}, program{0}, args{0}, closing{false}, reuseAddr{0}
{
	BUFSIZE = 0;
	BUFSIZE -= 2;
	dprint(BUFSIZE);
}

AccumServer::~AccumServer()
{
	closeServer();
}

void AccumServer::init(const std::string &settingsFile) throw (AccumException)
{
	nlohmann::json settings;

	std::string jsontext;

	std::fstream stream;
	stream.open(settingsFile.c_str(), std::ios_base::in);
	if (!stream.is_open())
		throw AccumException(AccumException::FILE_NEXS_EXC);
	while (!stream.eof())
	{
		static std::string temp;
		getline(stream, temp);
		jsontext += temp + '\n';
	}
	stream.close();

	try
	{
		settings = nlohmann::json::parse(jsontext);

		if (!settings["ipAddress"].is_null())
			ipAddress = settings["ipAddress"];
		if (!settings["port"].is_null())
			port = settings["port"];
		if (!settings["maxClientsNum"].is_null())
			maxClientsNum = settings["maxClientsNum"];
		if (!settings["whitelist"].is_null())
			for (auto el: settings["whitelist"])
				whitelist.push_back(el);
		if (!settings["blacklist"].is_null())
			for (auto el: settings["blacklist"])
				blacklist.push_back(el);
		if (!settings["workProgram"]["program"].is_null())
		{
			std::string name = settings["workProgram"]["program"];
			program = new char[name.size() + 1];
			strcpy(program, name.c_str());
		}
		if (!settings["workProgram"]["args"].is_null())
		{
			if (!settings["workProgram"]["args"].empty())
			{
				args = new char*[settings["workProgram"]["args"].size() + 2];
				int i = 1;
				for (auto el: settings["workProgram"]["args"])
				{
					std::string name = el;
					args[i] = new char[name.size() + 1];
					strcpy(args[i], name.c_str());
					dprint(args[i]);
					++i;
				}
				dprint(std::string("i = ") + std::to_string(i));
				args[i] = NULL;
				args[0] = program;
			}
		}
		if (!settings["pipePath"].is_null())
			pipePath = settings["pipePath"];
		if (!settings["reuseAddr"].is_null())
			reuseAddr = settings["reuseAddr"];
	}
	catch (...)
	{
		throw AccumException(AccumException::INV_JSON_EXC);
	}
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
	setsockopt(sockDescr, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddr, sizeof(reuseAddr));

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

	std::cout << "Started at " << ipAddress << ':' << port << std::endl << std::endl;

	// Рабочий поток
	std::thread workingThread = std::thread(&AccumServer::work, this);

	// Ждём, пока кто-либо не поставит closing в true
	while (true)
	{
		mutexClosing.lock();
		if (closing)
		{
			dprint("Im going to close serv");
			mutexClosing.unlock();
			closingServer();
			break;
		}
		mutexClosing.unlock();
	}

	dprint("Im waiting for workingthread");
	workingThread.join();
}


void AccumServer::work()
{
	while (opened && !closing)
	{
		struct sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);
		dprint("Waiting for connection");

		int clientSocket = accept(sockDescr, (struct sockaddr*)&clientAddr, (socklen_t*)&addrLen);

		if (clientSocket < 0)
		{
			dprint("Accept error");
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			break;
		}
		else if(closing || !opened)
		{
			dprint("Accept - closing server");
			break;
		}
		else
		{
			std::cout << "New connection:" << std::endl;
			std::cout << "IP: " << inet_ntoa(clientAddr.sin_addr) << std::endl;

			// Если есть непустой whitelist, то можно подключаться только тем, кто в whitelist-t
			if (!whitelist.empty())
			{
				if (std::find(whitelist.begin(), whitelist.end(), inet_ntoa(clientAddr.sin_addr)) == whitelist.end())
				{
					std::cout << "Connection rejected! Reason: IP is not in whitelist." << std::endl << std::endl;
					close(clientSocket);
					continue;
				}
			}
			// Если whitelist пустой (или отсутствует), то смотрим blacklist
			// Если он есть, то можно подключаться только тем, кто не в blacklist
			else if (!blacklist.empty())
			{
				if (std::find(whitelist.begin(), whitelist.end(), inet_ntoa(clientAddr.sin_addr)) == whitelist.end())
				{
					std::cout << "Connection rejected! Reason: IP is in blacklist." << std::endl << std::endl;
					close(clientSocket);
					continue;
				}
			}

			// Ждём, если все заняты
			while (curClientsNum >= maxClientsNum);


			mutexClients.lock();
			++curClientsNum;

			Client client;
			client.clientSocket = clientSocket;
			client.address = inet_ntoa(clientAddr.sin_addr);
			client.th = std::thread(&AccumServer::readData, this, clientSocket);
			client.th.detach();

			clients.push_back(std::move(client));
			mutexClients.unlock();
		}
	}
	dprint("WorkingThread leaving");
}

void AccumServer::closingServer()
{
	if (opened)
	{
		opened = false;

		std::cout << "Closing server... " << std::endl;
		// Отключаем все соединения
		close(sockDescr);
		// Шлём контрольный запрос для выхода из accept
		int tempSocket;
		struct sockaddr_in serverAddr;
		tempSocket = socket(AF_INET, SOCK_STREAM, 0);
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		inet_aton (ipAddress.c_str(), &serverAddr.sin_addr);
		connect(tempSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
		close(tempSocket);
		dprint("Closed tempSocket");
		// Чистим каталог с трубами
		DIR *pipeFolder = opendir(pipePath.c_str());
		dirent *file;
		while ((file = readdir(pipeFolder)) != NULL)
		{
			std::string filepath{pipePath + '/' + file->d_name};
			remove(filepath.c_str());
		}
		closedir(pipeFolder);
		dprint("pipes removed");
		// Даём watchDog-ам пару секунд, чтоб всё вырубили
		sleep(2);

		closing = false;

		std::cout << "Done." << std::endl;
	}
}

bool AccumServer::isOpened()
{
	return opened;
}

void AccumServer::readData(int clientSocket)
{
	int error = 0;

	pid_t pid;

	pid = fork();
	if (pid == -1) // Ошибка
	{
		error = 1;
	}
	else if (pid == 0) // Процесс потомок
	{
		dprint("Child");
		if (std::string(program).empty())
		{
			error = 2;
		}
		else
		{
			if (execv(program, args) == -1)
			{
				perror("execv");
				error = 3;
			}
		}
		dprint("Child OUT!!!");
	}
	else // Родительский процесс
	{
		dprint("Parent");
		// Создаём трубы
		int pipeDescr;

		std::string inputPipeName{pipePath + "/outputPipe_" + std::to_string(pid)};
		std::string outputPipeName{pipePath + "/inputPipe_" + std::to_string(pid)};
		if (mkfifo(inputPipeName.c_str(), 0777))
			error = 4;
		if (mkfifo(outputPipeName.c_str(), 0777))
			error = 4;

		dprint("All pipes were created!");

		if (error == 0)
		{
			bool stopWatchDog = false;
			// Создаём поток, следящий за работой дочернего процесса
			std::thread wthr(&AccumServer::watchDog, this, clientSocket, pid, std::ref(stopWatchDog));
			
			dprint("Before while");

			char *message = new char[BUFSIZE];
			// Вечный цикл обмена
			while (true)
			{
				dprint("Before making message");

				int bytesNumber = 0;

				dprint("reading...");
				// Читаем клиента
				//memset(message, '\0', BUFSIZE);
				message[0] = '\0';
				dprint("receiving...");
				bytesNumber = recv(clientSocket, message, BUFSIZE - 1, 0);
				message[bytesNumber] = '\0';
				dprint(std::string("Bytes!!!!! = ") + std::to_string(bytesNumber));
				if (bytesNumber <= 0 || bytesNumber > BUFSIZE - 1)
				{
					error = 6;
					break;
				}

				dprint("Got bytes");

				{
					int num = findClient(clientSocket);
					mutexClients.lock();
					std::cout << clients[num].address << std::endl << message << std::endl << std::endl;
					mutexClients.unlock();
				}

				if ((pipeDescr = open(outputPipeName.c_str(), O_WRONLY)) <= 0)
				{
					error = 5;
					break;
				}
				bytesNumber = write(pipeDescr, message, strlen(message));
				if (bytesNumber <= 0)
				{
					error = 7;
					break;
				}
				close(pipeDescr);

				//memset(message, '\0', BUFSIZE);
				message[0] = '\0';
				if ((pipeDescr = open(inputPipeName.c_str(), O_RDONLY)) <= 0)
				{
					error = 10;
					break;
				}

				bytesNumber = read(pipeDescr, message, BUFSIZE);
				dprint(std::string("BytesNumber = " + std::to_string(bytesNumber)));
				message[bytesNumber] = '\0';
				if (bytesNumber <= 0)
				{
					error = 8;
					break;
				}
				close(pipeDescr);

				dprint("Message from pipe:");
				dprint(message);
				dprint(std::string("length") + std::to_string(strlen(message)));
//				message[bytesNumber] = '\0';

				// Отправляем клиенту
				bytesNumber = send(clientSocket, message, bytesNumber, 0);
				dprint(std::string("sent = ") + std::to_string(bytesNumber));
				if (bytesNumber <= 0)
				{
					error = 9;
					break;
				}
			}

			delete[] message;
			message = 0;

			// Убиваем следящий поток
			stopWatchDog = true;
			wthr.join();
		}
		remove(inputPipeName.c_str());
		remove(outputPipeName.c_str());
		dprint("pipes were removed");
	}

	{
		int num = findClient(clientSocket);
		mutexClients.lock();
		std::cout << clients[num].address << ": ";
		mutexClients.unlock();
	}

	switch(error)
	{
		case 0:{
			std::cout << "Normal exit.";
			break;
		}
		case 1:{
			std::cout << "Error occurs while creating child process.";
			break;
		}
		case 2:{
			std::cout << "Work program is not defined in settings.";
			break;
		}
		case 3:{
			std::cout << "Error occurs on starting work program.";
			break;
		}
		case 4:{
			std::cout << "Error occurs on creating pipe.";
			break;
		}
		case 5:{
			std::cout << "Error occurs on opening output pipe.";
			break;
		}
		case 6:{
			std::cout << "Disconnected.";
			break;
		}
		case 7:{
			std::cout << "Error occurs while writing data through pipe.";
			break;
		}
		case 8:{
			std::cout << "Error occurs while reading data through pipe.";
			break;
		}
		case 9:{
			std::cout << "Error occurs on sending data to client.";
			break;
		}
		case 10:{
			std::cout << "Error occurs on opening input pipe.";
			break;
		}
	}

	std::cout << std::endl << std::endl;

	dprint("Leaving thread...");

	removeClient(clientSocket);
}

int AccumServer::findClient(int clientSocket)
{
	int n = 0;
	mutexClients.lock();
	for (auto it = clients.begin(); it != clients.end(); ++it, ++n)
		if (it->clientSocket == clientSocket)
			break;
	if (n == clients.size())
	{
		mutexClients.unlock();
		return -1;
	}
	else
	{
		mutexClients.unlock();
		return n;
	}
}

void AccumServer::removeClient(int clientSocket)
{
	int n = findClient(clientSocket);
	if (n == -1)
		return;
	close(clientSocket);

	mutexClients.lock();
	clients.erase(clients.begin() + n);
	--curClientsNum;
	mutexClients.unlock();
}

void AccumServer::watchDog(int clientSocket, pid_t pid, bool &stopWatchDog)
{
	dprint("Watch dog started");
	while (true)
	{
		// Рабочая программа завершилась с ошибкой
		if (waitpid(pid, NULL, WNOHANG) != 0)
		{
			int num = findClient(clientSocket);
			mutexClients.lock();
			std::cout << "Fatal! Working program crashed! (" << clients[num].address << ")" << std::endl;
			mutexClients.unlock();
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			dprint("Breakaus!");
			break;
		}

		// Сервер закрывается
		mutexClosing.lock();
		if (closing)
		{
			mutexClosing.unlock();
			if (waitpid(pid, NULL, WNOHANG) == 0)
			{
				dprint("Sending SIGTERM to python");
				kill(pid, SIGTERM);
				waitpid(pid, NULL, 0);
			}	
			break;
		}
		mutexClosing.unlock();

		// Клиент отключился
		mutexWatchDog.lock();
		if (stopWatchDog)
		{
			dprint("WatchDog was stopped");
			if (waitpid(pid, NULL, WNOHANG) == 0)
			{
				dprint("Sending SIGTERM to python");
				kill(pid, SIGTERM);
				waitpid(pid, NULL, 0);
			}
			mutexWatchDog.unlock();
			break;
		}
		mutexWatchDog.unlock();
	}
	dprint("Watch dog out!");
}

void AccumServer::catchCtrlC()
{
	dprint("I got Ctrl+C!!!");
	closeServer();
}

void AccumServer::closeServer()
{
	dprint("Call closeServer");
	closing = true;
}
