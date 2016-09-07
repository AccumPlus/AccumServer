#include <globals.h>
#include <errno.h>

#include "accumexception.h"

AccumException::AccumException(AccumExc exc):
	error(getExcMessage(exc))
{
}

const char* AccumException::what() const noexcept
{
	return error.c_str();
}

std::string AccumException::getExcMessage(AccumExc exc) const
{
	std::string message("***ACCUMEXCEPTION***: ");
	switch (exc)
	{
		case DEFAULT_EXC:
			message += "Other exception.";
		case SRV_CREAT_SOCK_EXC:
			message += "Exception while creating server-socket descriptor.";
		case SRV_BIND_PORT_EXC:
			message += "Port is already in use.";
		case SRV_BIND_IP_EXC:
			message += "IP address is not availiable.";
	}

	if (exc == DEFAULT_EXC)
		message += " Error code = " + std::to_string(errno);
	else
		message += " Exception code = " + std::to_string(exc);


	return message;
}

AccumException::~AccumException()
{
}
