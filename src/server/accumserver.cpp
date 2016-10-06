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
	maxClientsNum{1}, curClientsNum{0}, ipAddress{""}, port{0}, sockDescr{0}, opened{false}, program{0}, args{0}, closing{false}, reuseAddr{0}, logfile{""}
{
	BUFSIZE = 0;
	BUFSIZE -= 2;
}

AccumServer::~AccumServer()
{
	closeServer();
}

void AccumServer::init(const std::string &settingsFile) throw (AccumException)
{
	dprint("Func 'init()' started");
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
					++i;
				}
				args[i] = NULL;
				args[0] = program;
			}
		}
		if (!settings["pipePath"].is_null())
			pipePath = settings["pipePath"];
		if (!settings["reuseAddr"].is_null())
			reuseAddr = settings["reuseAddr"];
		if (!settings["logfile"].is_null())
			logfile = settings["logfile"];
	}
	catch (...)
	{
		throw AccumException(AccumException::INV_JSON_EXC);
	}

	if (!logfile.empty())
	{
		AccumLog::setWritingTypes(AccumLog::INF | AccumLog::WAR | AccumLog::ERR);
		AccumLog::start(logfile);
	}
	dprint("Func 'init()' stoped");
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
	dprint("Func 'openServer()' started");
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
			mutexClosing.unlock();
			closingServer();
			break;
		}
		mutexClosing.unlock();
	}

	workingThread.join();
	dprint("Func 'openServer()' stoped");
}


void AccumServer::work()
{
	dprint("Func 'work()' started");
	while (opened && !closing)
	{
		struct sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);

		int clientSocket = accept(sockDescr, (struct sockaddr*)&clientAddr, (socklen_t*)&addrLen);

		if (clientSocket < 0)
		{
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			break;
		}
		else if(closing || !opened)
		{
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
					AccumLog::writeLog(AccumLog::WAR, "Not whitelist connection tried to establish!");
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
					AccumLog::writeLog(AccumLog::WAR, "Blacklist connection tried to establish!");
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
	dprint("Func 'work()' stoped");
}

void AccumServer::closingServer()
{
	dprint("Func 'closingServer()' started");
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
		// Чистим каталог с трубами
		DIR *pipeFolder = opendir(pipePath.c_str());
		dirent *file;
		while ((file = readdir(pipeFolder)) != NULL)
		{
			std::string filepath{pipePath + '/' + file->d_name};
			remove(filepath.c_str());
		}
		closedir(pipeFolder);
		// Отключаем систему логов
		if (!logfile.empty())
			AccumLog::stop();
		// Даём watchDog-ам пару секунд, чтоб всё вырубили
		sleep(2);

		closing = false;

		std::cout << "Done." << std::endl;
	}
	dprint("Func 'closingServer()' stoped");
}

bool AccumServer::isOpened()
{
	return opened;
}

void AccumServer::readData(int clientSocket)
{
	dprint("Func 'readData()' started");
	int error = 0;

	pid_t pid;

	pid = fork();
	if (pid == -1) // Ошибка
	{
		error = 1;
	}
	else if (pid == 0) // Процесс потомок
	{
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
	}
	else // Родительский процесс
	{
		// Создаём трубы
		int pipeDescr;

		std::string inputPipeName{pipePath + "/outputPipe_" + std::to_string(pid)};
		std::string outputPipeName{pipePath + "/inputPipe_" + std::to_string(pid)};
		if (mkfifo(inputPipeName.c_str(), 0777))
			error = 4;
		if (mkfifo(outputPipeName.c_str(), 0777))
			error = 4;

		if (error == 0)
		{
			bool stopWatchDog = false;
			// Создаём поток, следящий за работой дочернего процесса
			std::thread wthr(&AccumServer::watchDog, this, clientSocket, pid, std::ref(stopWatchDog));
			
			char *message = new char[BUFSIZE];
			// Вечный цикл обмена
			while (true)
			{
				int bytesNumber = 0;

				// Читаем клиента
				//memset(message, '\0', BUFSIZE);
				message[0] = '\0';
				bytesNumber = recv(clientSocket, message, BUFSIZE - 1, 0);
				message[bytesNumber] = '\0';
				if (bytesNumber <= 0 || bytesNumber > BUFSIZE - 1)
				{
					error = 6;
					break;
				}

				{
					int num = findClient(clientSocket);
					mutexClients.lock();
					std::cout << clients[num].address << std::endl << message << std::endl << std::endl;
					AccumLog::writeLog(AccumLog::INF, message);
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

				message[0] = '\0';
				if ((pipeDescr = open(inputPipeName.c_str(), O_RDONLY)) <= 0)
				{
					error = 10;
					break;
				}

				bytesNumber = read(pipeDescr, message, BUFSIZE);
				message[bytesNumber] = '\0';
				if (bytesNumber <= 0)
				{
					error = 8;
					break;
				}
				close(pipeDescr);

				// Отправляем клиенту
				bytesNumber = send(clientSocket, message, bytesNumber, 0);
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
			// По-хорошему, сюда не должно попасть никогда
			std::cout << "Normal exit.";
			break;
		}
		case 1:{
			std::cout << "Error occurs while creating child process.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs while creating child process.");
			break;
		}
		case 2:{
			std::cout << "Work program is not defined in settings.";
			AccumLog::writeLog(AccumLog::ERR, "Work program is not defined in settings.");
			break;
		}
		case 3:{
			std::cout << "Error occurs on starting work program.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs on starting work program.");
			break;
		}
		case 4:{
			std::cout << "Error occurs on creating pipe.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs on creating pipe.");
			break;
		}
		case 5:{
			std::cout << "Error occurs on opening output pipe.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs on opening output pipe.");
			break;
		}
		case 6:{
			// Нормальный выход
			std::cout << "Disconnected.";
			break;
		}
		case 7:{
			std::cout << "Error occurs while writing data through pipe.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs while writing data through pipe.");
			break;
		}
		case 8:{
			std::cout << "Error occurs while reading data through pipe.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs while reading data through pipe.");
			break;
		}
		case 9:{
			std::cout << "Error occurs on sending data to client.";
			AccumLog::writeLog(AccumLog::WAR, "Error occurs on sending data to client.");
			break;
		}
		case 10:{
			std::cout << "Error occurs on opening input pipe.";
			AccumLog::writeLog(AccumLog::ERR, "Error occurs on opening input pipe.");
			break;
		}
	}

	std::cout << std::endl << std::endl;

	removeClient(clientSocket);
	dprint("Func 'readData()' stoped");
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
	dprint("Func 'watchDog()' started");
	while (true)
	{
		// Рабочая программа завершилась с ошибкой
		if (waitpid(pid, NULL, WNOHANG) != 0)
		{
			int num = findClient(clientSocket);
			mutexClients.lock();
			std::cout << "Fatal! Working program crashed! (" << clients[num].address << ")" << std::endl;
			AccumLog::writeLog(AccumLog::ERR, std::string("Fatal! Working program crashed! (") + clients[num].address + ")");
			mutexClients.unlock();
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			break;
		}

		// Сервер закрывается
		mutexClosing.lock();
		if (closing)
		{
			mutexClosing.unlock();
			if (waitpid(pid, NULL, WNOHANG) == 0)
			{
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
			if (waitpid(pid, NULL, WNOHANG) == 0)
			{
				kill(pid, SIGTERM);
				waitpid(pid, NULL, 0);
			}
			mutexWatchDog.unlock();
			break;
		}
		mutexWatchDog.unlock();
	}
	dprint("Func 'watchDog()' stoped");
}

void AccumServer::catchCtrlC()
{
	dprint("Func 'catchCtrlC()' started");
	closeServer();
	dprint("Func 'catchCtrlC()' stoped");
}

void AccumServer::closeServer()
{
	dprint("Func 'closeServer()' started");
	closing = true;
	dprint("Func 'closeServer()' stoped");
}
