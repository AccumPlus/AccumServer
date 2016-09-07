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
							SRV_BIND_IP_EXC			= 103
		};
		AccumException(AccumExc exc);
		virtual const char* what() const noexcept;
		~AccumException();
	private:
		std::string getExcMessage(AccumExc exc) const;
		std::string error;
};

#endif
