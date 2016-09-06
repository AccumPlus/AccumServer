#ifndef SERVER_H
#define SERVER_H

#include <string>

class Server
{
	public:
		enum Error:int {	NO_ERR					= 0,
							SRV_CREATE_ERR			= 101,
							SRV_BIND_ERR			= 102,
							SRV_LISTEN_ERR			= 103,
							INV_IP_ERR				= 201,
							INV_NOIP_ERR			= 202
		};
		Server();
		~Server();

		void setIpAddress(const std::string &ipAddress);
		void setPort(const short &port);
		void setMaxClientsNum(const int &num);

		std::string getIpAddress();
		short getPort();
		int getMaxClientsNum();

		int open();
		void close();

	private:
		std::string ipAddress;
		short port;
		int maxClientsNum;

		int sockDescr;
};

#endif
