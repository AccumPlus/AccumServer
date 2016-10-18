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
#include <queue>
#include <map>

#include <exception/accumexception.h>
#include <log/accumlog.h>
#include <json.hpp>

class AccumServer
{
	public:
		AccumServer();
		~AccumServer();

		// Функция инициализации сервера (загрузка конфига и т.д.)
		void init(const std::string &settingsFile) throw (AccumException);
		// Функция обработки SIGINT сигнала
		void catchCtrlC();

		// Функция открытия сервера
		int openServer() throw (AccumException);
		// Функция закрытия сервера (установка флага)
		void closeServer();
	private:
		// Основная функция ловли клиентов
		void work();
		// Функция обработки клиентских запросов
		void readData(int number);
		// Функция слежки за рабочим приложением
		void watchDog(int number, bool &restart, AccumException::AccumExc &error);
		// Функция закрытия сервера
		void closingServer();

		// Перменная, хранящая размер буфера для сообщений
		unsigned short BUFSIZE;

		// === ХАРАКТЕРИСТИКИ СЕРВЕРА ===
		int sockDescr;						// Дескриптор сервер-сокета
		sockaddr_in srvAddr;				// Структура для хранения адреса сервера
		std::string ipAddress;				// IP адрес
		short port;							// Порт
		int maxClientsNum;					// Максимальное число подключений
		std::vector<std::string> whitelist;	// "Белый" список IP адресов (подключаться можно только им)
		std::vector<std::string> blacklist;	// "Чёрный" список IP адресов (им подключаться нельзя)
		char *program;						// Программа-обработчик запросов
		char **args;						// Аргументы для программы-обработчика
		std::string pipePath;				// Путь к каталогу с трубами
		int reuseAddr;						// Разрешение на мгновенное повторное использование адреса после освобождения
		std::string logfile;				// Пусть к log-файлу
		std::map<std::string, 
			std::string> defaultReplyes;	// Стандартные ответы клиенту
		// ==============================

		bool opened;						// Флаг открыт ли сервер
		std::mutex mutexOpened;				// Мьютекс на изменение флага открытия
		bool closing;						// Флаг закрывается ли сервер
		std::mutex mutexClosing;			// Мьютекс на изменене флага закрытия
		unsigned int curClientsNum;			// Текущее количество клиентов
		std::mutex mutexClients;			// Мьютекс на изменение текущего количества

		std::queue<int> clientsQueue;		// Очередь сокетов клиентов, готовых к обработке
		std::mutex mutexQueue;				// Мьютекс на изменение очереди клиентов

		std::vector<pid_t> processes;		// Вектор идентификаторов процессов
		std::mutex mutexProcesses;			// Мьютекс на изменение вектора идетификаторов процессов
};

#endif
