#include <algorithm>
#include <thread>
#include <ctime>
#include <chrono>
#include <iomanip>

#include "../globals.h"

#include "accumlog.h"

std::queue<std::string> AccumLog::logQueue;
bool AccumLog::started = false;
int AccumLog::curFlags = 0x06;
std::ofstream AccumLog::ostrm;
bool AccumLog::closing = false;
bool AccumLog::closed = true;
bool AccumLog::openning = false;

std::mutex AccumLog::mutexQueue;
std::mutex AccumLog::mutexClosing;
std::mutex AccumLog::mutexClosed;
std::mutex AccumLog::mutexOpenning;
std::map<int, std::string> AccumLog::flagTypes;

AccumLog::~AccumLog()
{
	stop();
}

void AccumLog::start(std::string filename) throw (AccumException)
{
	if (flagTypes.empty())
	{
		flagTypes[INF] = "INF";
		flagTypes[WAR] = "WAR";
		flagTypes[ERR] = "ERR";
	}
	// Выставляем флаг "открывается", чтобы никто не попытался записать, пока файл не открыт
	mutexOpenning.lock();
	openning = true;
	mutexOpenning.unlock();
	
	ostrm.open(filename.c_str(), std::ios_base::out | std::ios_base::app);
	if (!ostrm.is_open())
		throw AccumException(AccumException::FILE_NEXS_EXC);

	// Когда открылся, выставляем флаг "открыт"
	mutexOpenning.lock();
	openning = false;
	mutexOpenning.unlock();
	mutexClosed.lock();
	closed = false;
	mutexClosed.unlock();

	std::thread workingThread = std::thread(&AccumLog::work);
	workingThread.detach();
}

void AccumLog::stop()
{
	// Если логгер открывается, то ждём пока откроется
	mutexOpenning.lock();
	if (openning)
	{
		mutexOpenning.unlock();
		// Ждём, пока откроется
		while (true)
		{
			mutexClosed.lock();
			if (!closed)
			{
				mutexClosed.unlock();
				break;
			}
			mutexClosed.unlock();
		}
	}
	else
	{
		mutexOpenning.unlock();
	}

	// Если логгер уже закрывается, то ливаем
	mutexClosing.lock();
	if (closing)
	{
		mutexClosing.unlock();
		return;
	}
	// Выставляем флаг "Закрывается"
	closing = true;
	mutexClosing.unlock();
	
	// Ждём пока дочерний поток закинет всю очередь в файл
	while (true)
	{
		mutexClosed.lock();
		if (closed)
		{
			mutexClosed.unlock();
			break;
		}
		mutexClosed.unlock();
	}
	// Закрываем файловый поток
	ostrm.close();

}

void AccumLog::writeLog(TYPES type, std::string message)
{
	// Проверяем, открывается ли логгер
	mutexOpenning.lock();
	if (openning)
	{
		mutexOpenning.unlock();
		// Ждём, пока откроется
		while (true)
		{
			mutexOpenning.lock();
			if (!openning)
			{
				mutexOpenning.unlock();
				break;
			}
			mutexOpenning.unlock();
		}
	}
	else
		mutexOpenning.unlock();

	// Если сервер закрыт или закрывается - ливаем
	mutexClosing.lock();
	mutexClosed.lock();
	if (closing || closed)
		return;
	mutexClosing.unlock();
	mutexClosed.unlock();

	// Нужно ли писать этот тип сообщения
	if (type & curFlags)
	{
		std::replace(message.begin(), message.end(), '\n', ' ');
		std::replace(message.begin(), message.end(), '\r', ' ');
		mutexQueue.lock();
		logQueue.push(flagTypes[type] + " | " + message);
		mutexQueue.unlock();
	}
}

void AccumLog::setWritingTypes(int flags)
{
	curFlags = flags;
}

void AccumLog::work()
{
	int qSize = 0;
	while (true)
	{
		mutexQueue.lock();
		qSize = logQueue.size();

		if (qSize > 0)
		{
			dprint(std::string("Qsize = ") + std::to_string(qSize));
			std::string mes = logQueue.front();
			logQueue.pop();
			mutexQueue.unlock();

			std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
			std::time_t now_c = std::chrono::system_clock::to_time_t(now);
			std::tm now_tm = *std::localtime(&now_c);
			char buf[70];
			strftime(buf, 70, "%d.%m.%Y %H:%M:%S", &now_tm);
			mes = std::string(buf) + " | " + mes + '\n';

			ostrm << mes;
			ostrm.flush();
		}
		else // Если очередь пустая, то проверяем, не закрывается ли логгер
		{
			AccumLog::mutexClosing.lock();
			if (closing)
			{
				mutexClosing.unlock();
				mutexQueue.unlock();
				break;
			}
			mutexClosing.unlock();
			mutexQueue.unlock();
		}
	}
	mutexClosed.lock();
	closed = true;
	mutexClosed.unlock();
}
