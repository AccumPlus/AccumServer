#ifndef ACCUMLOG_H
#define ACCUMLOG_H

#include <queue>
#include <string>
#include <mutex>
#include <fstream>
#include <map>

#include <exception/accumexception.h>

class AccumLog
{
	public:
		AccumLog();
		~AccumLog();

		enum TYPES:int {
			INF =	0x01,
			WAR =	0x02,
			ERR =	0x03
		};

		static void setWritingTypes(int flags);
		static void writeLog(TYPES type, std::string message);
		static void start(std::string filename) throw (AccumException);
		static void stop();

	private:
		static std::queue<std::string> logQueue;
		static bool started;
		static int curFlags;
		static std::ofstream ostrm;
		static bool closing;
		static bool closed;
		static bool openning;
		static std::map<int, std::string> flagTypes;

		static std::mutex mutexQueue;
		static std::mutex mutexClosing;
		static std::mutex mutexClosed;
		static std::mutex mutexOpenning;
		
		static void work();
};

#endif
