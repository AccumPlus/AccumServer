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
	maxClientsNum{1}, ipAddress{""}, port{0}, sockDescr{0}, opened{false}, program{0}, args{0}, closing{false}, reuseAddr{0}, logfile{""}, curClientsNum{0}
{
	BUFSIZE = 5242880; // 5 Mb
	//BUFSIZE -= 2;
}

AccumServer::~AccumServer()
{
	//closeServer();
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
		if (!settings["defaultReplyes"].is_null())
			if (!settings["defaultReplyes"]["timeout"].is_null())
				defaultReplyes["timeout"] = settings["defaultReplyes"]["timeout"];

	}
	catch (...)
	{
		throw AccumException(AccumException::INV_JSON_EXC);
	}

	std::regex pattern("^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
	if (!std::regex_match(ipAddress, pattern))
		throw AccumException(AccumException::INV_IP_STR_EXC);
	if (maxClientsNum < 0)
		throw AccumException(AccumException::INV_CL_NUM_EXC);

	processes = std::vector<pid_t>(maxClientsNum, 0);

	if (!logfile.empty())
	{
		AccumLog::setWritingTypes(AccumLog::INF | AccumLog::WAR | AccumLog::ERR);
		AccumLog::start(logfile);
	}
	dprint("Func 'init()' stoped");
}

int AccumServer::openServer() throw (AccumException)
{
	dprint("Func 'openServer()' started");

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

	std::thread *clientThreads = new std::thread[maxClientsNum];

	// Запускаем N потоков с клиентскими обработками
	for (int i = 0; i < maxClientsNum; ++i)
		clientThreads[i] = std::thread(&AccumServer::readData, this, i);

	opened = true;


	// Запускаем поток с рабочей функцией
	std::thread workingThread = std::thread(&AccumServer::work, this);

	std::cout << "Started at " << ipAddress << ':' << port << std::endl << std::endl;

	AccumLog::writeLog(AccumLog::INF, std::string("Server was started at ") + ipAddress + ':' + std::to_string(port) + '!');

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

	dprint("Waiting for working thread...");
	workingThread.join();
	dprint("Waiting for clients thread...");
	for (int i = 0; i < maxClientsNum; ++i)
		clientThreads[i].join();

	dprint("Func 'openServer()' stoped");
}

void AccumServer::work()
{
	dprint("Func 'work()' started");
	while (opened && !closing)
	{
		// Ловим клиента
		struct sockaddr_in clientAddr;
		int addrLen = sizeof(clientAddr);

		int clientSocket = accept(sockDescr, (struct sockaddr*)&clientAddr, (socklen_t*)&addrLen);

		// Ошибка
		if (clientSocket < 0)
		{
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			break;
		}
		else if(closing || !opened) // Сюда попадаем, когда получаем контрольное подключение
		{
			break;
		}
		else // Всё нормально
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
			while (true)
			{
				mutexClients.lock();
				if (curClientsNum < maxClientsNum)
				{
					dprint("Here is free slot!");
					curClientsNum++;
					mutexClients.unlock();
					break;
				}
				mutexClients.unlock();
			}

			// Записываем ip-адрес
			mutexIpClients.lock();
			ipClients[clientSocket] = inet_ntoa(clientAddr.sin_addr);
			mutexIpClients.unlock();

			// Заполняем очередь клиентов
			mutexQueue.lock();
			clientsQueue.push(clientSocket);
			mutexQueue.unlock();

		}
	}
	dprint("Func 'work()' stoped");
}

void AccumServer::readData(int number)
{
	dprint("Func 'readData()' started");
	// Запускаем watchDog-а
	bool restart = false;
	AccumException::AccumExc watchDogError = AccumException::UNDEFINED;
	std::thread watchDoggy = std::thread(&AccumServer::watchDog, this, number, std::ref(restart), std::ref(watchDogError));

	bool leaveLoop = false;

	int clientSocket;
	char *message = new char[BUFSIZE];
	
	do
	{
		clientSocket = 0;

		try
		{
			dprint("Waiting for connection...");
			// Ожидаем заполнение очереди
			while (true)
			{
				mutexQueue.lock();
				mutexClosing.lock();
				mutexOpened.lock();

				
				if (clientsQueue.size() > 0 && !closing && opened)
				{
					dprint("Client was catched!");
					clientSocket = clientsQueue.front();
					clientsQueue.pop();

					mutexClosing.unlock();
					mutexOpened.unlock();
					mutexQueue.unlock();
					break;
				}
				else if (closing || !opened) // Сервер закрывается
				{
					dprint("Read function closing.......!");
					mutexQueue.unlock();
					mutexClosing.unlock();
					mutexOpened.unlock();
					leaveLoop = true;
					break;
	//				throw AccumException(AccumException::CLOSE_SERV);
				}

				/*dprint("closing:");
				dprint(closing);
				dprint("opened:");
				dprint(opened);
				sleep(1);*/

				mutexQueue.unlock();
				mutexClosing.unlock();
				mutexOpened.unlock();
			}

			if (leaveLoop)
			{
				std::cout << "Read function ended by closing server!" << std::endl;
				break;
			}

			// Ждём какой-то реакции от WatchDog-а
			while (watchDogError == AccumException::UNDEFINED){;}
			if (watchDogError != AccumException::NO_ERR)
				throw AccumException((AccumException::AccumExc)watchDogError);

			// Идентификатор процесса
			pid_t pid;

			// Названия труб
			std::string inputPipeName;
			std::string outputPipeName;
			
			int pipeDescr = 0;
			struct timeval timeout;
			fd_set set;
			// Вечный цикл обмена
			while (true)
			{
				int bytesNumber = 0;

				/* Новое чтение */
				// Настраиваем таймер

				while (1)
				{
					FD_ZERO(&set);
					FD_SET(clientSocket, &set);
					timeout.tv_sec = 2;
					timeout.tv_usec = 0;
					// Ждём изменений на сокете
					bytesNumber = select(clientSocket + 1, &set, NULL, NULL, &timeout);
					if (bytesNumber == -1) // error
					{
						dprint("error!!!__!__!");
						throw AccumException::SELECT_ERR;
					}
					else if (bytesNumber == 0) // timeout
					{
						mutexClosing.lock();
						mutexOpened.lock();

						if (closing || !opened) // Сервер закрывается
						{
							leaveLoop = true;
							mutexClosing.unlock();
							mutexOpened.unlock();
							break;
						}

						mutexClosing.unlock();
						mutexOpened.unlock();
					}
					else // normal
					{
						bytesNumber = recv(clientSocket, message, BUFSIZE - 1, 0);
						message[bytesNumber] = '\0';
						break;
					}
				}

				if (bytesNumber <= 0 || bytesNumber > BUFSIZE - 1)
				{
					mutexIpClients.lock();
					std::cout << ipClients[clientSocket] << ":\n" << "Disconnected!" << std::endl;
					mutexIpClients.unlock();
					break;
				}
				dprint(std::string("mes from socket got: ") + message);

				if (leaveLoop)
					break;

				// Читаем клиента
	/*			dprint("getting mes from socket...");
				message[0] = '\0';
				bytesNumber = recv(clientSocket, message, BUFSIZE - 1, 0);
				dprint(std::string("mes from socket got: ") + message);
				message[bytesNumber] = '\0';
				dprint(bytesNumber);
				if (bytesNumber <= 0 || bytesNumber > BUFSIZE - 1)
				{
					mutexIpClients.lock();
					std::cout << ipClients[clientSocket] << ":\n" << "Disconnected!" << std::endl;
					mutexIpClients.unlock();
					break;
				}
				dprint(std::string("mes from socket got: ") + message);*/

				// Выводим на экран
				mutexIpClients.lock();
				std::cout << ipClients[clientSocket] << ":\n" << message << std::endl;
				AccumLog::writeLog(AccumLog::INF, std::string("Data from ") + ipClients[clientSocket] + ": " + message);
				mutexIpClients.unlock();

				// Получаем названия труб
				mutexProcesses.lock();
				pid = processes[number];
				mutexProcesses.unlock();
				inputPipeName = pipePath + "/outputPipe_" + std::to_string(pid);
				outputPipeName = pipePath + "/inputPipe_" + std::to_string(pid);

				// Пишем в трубу
				dprint("sending mes to pipe...");
				if ((pipeDescr = open(outputPipeName.c_str(), O_WRONLY)) <= 0)
					throw AccumException(AccumException::OPEN_OPIPE_ERR);
				bytesNumber = write(pipeDescr, message, strlen(message));
				if (bytesNumber <= 0)
					throw AccumException(AccumException::WRITE_PIPE_ERR);
				close(pipeDescr);
				dprint("mes to pipe sent");

				// Читаем трубу
				dprint("getting mes from pipe...");
				message[0] = '\0';
				if ((pipeDescr = open(inputPipeName.c_str(), O_RDONLY | O_NONBLOCK)) <= 0)
					throw AccumException(AccumException::OPEN_IPIPE_ERR);

				dprint("Go go go!");
				FD_ZERO(&set);
				FD_SET(pipeDescr, &set);
				timeout.tv_sec = 10;
				timeout.tv_usec = 0;

	//			bool timeOutFlag = false;
				bytesNumber = select(pipeDescr + 1, &set, NULL, NULL, &timeout);
				if (bytesNumber == -1) // error
				{
					dprint("error!!!__!__!");
					throw AccumException::SELECT_ERR;
				}
				else if (bytesNumber == 0) // timeout
				{
					dprint("Timeout!!!");
					strcpy(message, defaultReplyes["timeout"].c_str());
					dprint(message);
					bytesNumber = strlen(message);
					restart = true;
				}
				else // normal
				{
					bytesNumber = read(pipeDescr, message, BUFSIZE);
					message[bytesNumber] = '\0';
					if (bytesNumber <= 0)
						throw AccumException(AccumException::READ_PIPE_ERR);
					close(pipeDescr);
					dprint("mes from pipe got");
				}

				dprint("sending mes to socket...");
				// Отправляем клиенту
				bytesNumber = send(clientSocket, message, bytesNumber, 0);
				if (bytesNumber <= 0)
					throw AccumException(AccumException::SEND_SOCK_ERR);
				dprint("mes to socket sent");
			}
			dprint("exchange loop out!");

		}
		catch (AccumException &e)
		{
			mutexIpClients.lock();
			std::string mes = {""};
			if (ipClients.count(clientSocket) > 0)
				mes += ipClients[clientSocket] + ":\n";
			std::cout << mes + e.what() << std::endl;
			if (e.getCode() == AccumException::NO_ERR ||
					e.getCode() == AccumException::CLOSE_SERV ||
					e.getCode() == AccumException::DISCONNECT)
				AccumLog::writeLog(AccumLog::INF, mes + e.what());
			else
				AccumLog::writeLog(AccumLog::ERR, mes + e.what());
			mutexIpClients.unlock();
		}


		close(clientSocket);

		mutexClients.lock();
		curClientsNum--;
		mutexClients.unlock();

		mutexIpClients.lock();
		ipClients.erase(clientSocket);
		mutexIpClients.unlock();

	/*	if (leaveLoop)
			break;*/
	}
	while (true);

	delete[] message;
	message = 0;

	// Ждём завершение следящего потока
	watchDoggy.join();

	dprint("Func 'readData()' stoped");
}

void AccumServer::watchDog(int number, bool &restart, AccumException::AccumExc &error)
{
	dprint("Func 'watchDog()' started");

	bool run = true;
	while (run)
	{
		error = AccumException::UNDEFINED;
		pid_t pid = fork();


		if (pid == -1) // Ошибка
		{
			error = AccumException::CREAT_CHILD_ERR;
			mutexClosing.lock();
			closing = true;
			mutexClosing.unlock();
			break;
		}
		else if (pid == 0) // Процесс потомок
		{
			if (std::string(program).empty())
			{
				error = AccumException::NO_WPROGRAM_ERR;
				mutexClosing.lock();
				closing = true;
				mutexClosing.unlock();
				break;
			}
			else
			{
				if (execv(program, args) == -1)
				{
					perror("execv");
					error = AccumException::START_WPROG_ERR;
					mutexClosing.lock();
					closing = true;
					mutexClosing.unlock();
					break;
				}
			}
		}
		else // Родительский процесс
		{
			mutexProcesses.lock();
			processes[number] = pid;
			mutexProcesses.unlock();
			updateProcesses();

			std::string inputPipeName{pipePath + "/outputPipe_" + std::to_string(pid)};
			std::string outputPipeName{pipePath + "/inputPipe_" + std::to_string(pid)};
			if (mkfifo(inputPipeName.c_str(), 0777) or mkfifo(outputPipeName.c_str(), 0777))
			{
				error = AccumException::CREAT_PIPE_ERR;
				mutexClosing.lock();
				closing = true;
				mutexClosing.unlock();
				break;
			}

			error = AccumException::NO_ERR;

			while (true)
			{
				// Рабочая программа завершилась с ошибкой
				if (waitpid(pid, NULL, WNOHANG) != 0)
				{
					std::cout << "Fatal! Working program crashed!" << std::endl;
					mutexProcesses.lock();
					processes[number] = 0;
					mutexProcesses.unlock();
					updateProcesses();
					AccumLog::writeLog(AccumLog::ERR, std::string("Fatal! Working program crashed!"));
					break;
				}

				// Сервер закрывается
				mutexClosing.lock();
				if (closing)
				{
					mutexProcesses.lock();
					processes[number] = 0;
					mutexProcesses.unlock();
					updateProcesses();

					mutexClosing.unlock();
					if (waitpid(pid, NULL, WNOHANG) == 0)
					{
						kill(pid, SIGTERM);
						waitpid(pid, NULL, 0);
					}
					run = false;	
					break;
				}
				mutexClosing.unlock();
				
				// Просят перезапустить
				if (restart)
				{
					mutexProcesses.lock();
					processes[number] = 0;
					mutexProcesses.unlock();
					updateProcesses();

					if (waitpid(pid, NULL, WNOHANG) == 0)
					{
						kill(pid, SIGTERM);
						waitpid(pid, NULL, 0);
					}
					restart = false;
					break;
				}
			}

			remove(inputPipeName.c_str());
			remove(outputPipeName.c_str());
		}
	}
	dprint("Func 'watchDog()' stoped");
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
		AccumLog::writeLog(AccumLog::INF, std::string("Server was closed!"));
		if (!logfile.empty())
			AccumLog::stop();
		// Даём watchDog-ам пару секунд, чтоб всё вырубили
		sleep(2);

		closing = false;

		std::cout << "Done." << std::endl;
	}
	dprint("Func 'closingServer()' stoped");
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

void AccumServer::updateProcesses()
{
	dprint("Func 'updateProcesses()' started");
	mutexProcesses.lock();
	std::fstream stream;
	stream.open("processespids", std::ios_base::out | std::ios_base::trunc);
	for(int pid: processes)
		stream << pid << std::endl;
	stream.close();
	mutexProcesses.unlock();
	dprint("Func 'updateProcesses()' stoped");
}
