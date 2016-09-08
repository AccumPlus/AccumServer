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
			break;
		case SRV_CREAT_SOCK_EXC:
			message += "Exception while creating server-socket descriptor.";
			break;
		case SRV_BIND_PORT_EXC:
			message += "Port is already in use.";
			break;
		case SRV_BIND_IP_EXC:
			message += "IP address is not availiable.";
			break;
		case SRV_LIST_EXC:
			message += "Exception at listening activation.";
			break;
		case INV_CL_NUM_EXC:
			message += "Invalid maximum clients number.";
			break;
		case INV_IP_STR_EXC:
			message += "Invalid IP address structure.";
			break;
		case SRV_ACCEPT_EXC:
			message += "Exception at acception client's connection.";
			break;
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
