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
		case NO_ERR:
			message += "No error.";
			break;
		case CLOSE_SERV:
			message += "Called closing server.";
			break;
		case DISCONNECT:
			message += "Disconnected";
			break;
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
		case INV_JSON_EXC:
			message += "Bad json file.";
			break;
		case FILE_NEXS_EXC:
			message += "File not exists.";
			break;
		case CREAT_CHILD_ERR:
			message += "Error occurs while creating child process.";
			break;
		case NO_WPROGRAM_ERR:
			message += "Work program is not defined in settings.";
			break;
		case START_WPROG_ERR:
			message += "Error occurs on starting work program.";
			break;
		case CREAT_PIPE_ERR:
			message += "Error occurs on creating pipe.";
			break;
		case OPEN_IPIPE_ERR:
			message += "Error occurs on opening input pipe.";
			break;
		case WRITE_PIPE_ERR:
			message += "Error occurs while writing data through pipe.";
			break;
		case READ_PIPE_ERR:
			message += "Error occurs while reading data through pipe.";
			break;
		case SEND_SOCK_ERR:
			message += "Error occurs on sending data to client.";
			break;
		case OPEN_OPIPE_ERR:
			message += "Error occurs on opening output pipe.";
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
