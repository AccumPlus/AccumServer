#ifndef ACCUMEXCEPTION_H
#define ACCUMEXCEPTION_H

#include <exception>
#include <string>


class AccumException: public std::exception
{
	public:
		enum AccumExc:int{	DEFAULT_EXC				= 0,
							SRV_CREAT_SOCK_EXC		= 101,
							SRV_BIND_PORT_EXC		= 102,
							SRV_BIND_IP_EXC			= 103,
							SRV_LIST_EXC			= 104,
							SRV_ACCEPT_EXC			= 105,
							INV_CL_NUM_EXC			= 201,
							INV_IP_STR_EXC			= 202,
							INV_JSON_EXC			= 203,
							FILE_NEXS_EXC			= 304
		};
		AccumException(AccumExc exc);
		virtual const char* what() const noexcept;
		~AccumException();
	private:
		std::string getExcMessage(AccumExc exc) const;
		std::string error;
};

#endif
