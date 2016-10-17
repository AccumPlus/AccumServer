#include <iostream>
#include <json.hpp>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "globals.h"
#include "server/accumserver.h"
#include "exception/accumexception.h"

void catchChildSig(int s);

void catchParSig(int s);

int run();

AccumServer server;

pid_t pid;

bool stop = false;

int main(int argc, char**argv)
{
	std::cout << "=== AccumServer ===" << std::endl << "Version: " << VERSION << std::endl;
	bool daemon = false;

	if (argc > 1)
	{
		if (strcmp(argv[1], "-d") == 0)
		{
			dprint("daemon activated!");
			daemon = true;
		}
	}

	pid_t lpid;

	if (daemon)
	{
		// Отсоединяем от терминала
		close(1); // stdout
		close(2); // stderr

		int devNull = open("/dev/null", O_RDWR);

		dup(devNull);
		dup(devNull);

		lpid = fork();
		if (lpid == -1)
		{
			std::cout << "Couldn't start daemon!" << std::endl;
			return 1;
		}
		else if (lpid == 0) // Потомок
		{
			setsid();

			while (true)
			{
				pid = fork();
				if (pid == -1)
				{
					std::cout << "Couldn't start server!" << std::endl;
					return 1;
				}
				else if (pid == 0) // Потомок (работа на нём)
				{
					return run();
				}
				else // Родитель - watchDog
				{
					dprint(std::string("child pid = ") + std::to_string(pid));
					struct sigaction signalHandler;
					signalHandler.sa_handler = catchParSig;
					sigemptyset(&signalHandler.sa_mask);
					signalHandler.sa_flags = 0;
					sigaction(SIGINT, &signalHandler, NULL);
					sigaction(SIGTERM, &signalHandler, NULL);
					
					int status;
					waitpid(pid, &status, 0);
					// Завершился через return
					if (WIFEXITED(status))
					{
						// Смотрим, что вернуло
						int st = WEXITSTATUS(status);
						switch (st)
						{
							default:
							case 1: // Exception. На эксепшнах висят жёсткие исключения, связанные с запуском сервера. Перезапуск не поможет.
							{
								stop = true;
								break;
							}
/*							case 2: // Ошибка в рабочей программе. Считаем, что перезапуск может помочь.
							{
								stop = false;
								break;
							}*/
							case 0: // Нормальное завершение (посыл SIGINT/SIGTERM). Перезапускать не нужно.
							{
								stop = true;
								break;
							}
						}
					}
					else // Завершился без return-а (Жёсткая ошибка). Пробуем вылечить перезапуском.
					{
						stop = false;
					}
					if (stop)
						break;
				}
			}
		}

		close(devNull);
	}
	else
	{
		return run();
	}
	
	return 0;
}

void catchChildSig(int s)
{
	server.catchCtrlC();
}

void catchParSig(int s)
{
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
	stop = true;
}


int run()
{
	struct sigaction signalHandler;
	signalHandler.sa_handler = catchChildSig;
	sigemptyset(&signalHandler.sa_mask);
	signalHandler.sa_flags = 0;
	sigaction(SIGINT, &signalHandler, NULL);
	sigaction(SIGTERM, &signalHandler, NULL);

	int ret = 0;

	try
	{
		server.init("../etc/accumserver.json");
		server.openServer();
	}
	catch(AccumException &e)
	{
		std::cout << e.what() << std::endl;
		ret = 1;
	}

	/* По-хорошему, здесь сервер уже закрыт */
	server.closeServer();
	return ret;
}
