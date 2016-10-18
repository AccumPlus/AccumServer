#ifndef ACCUMEXCEPTION_H
#define ACCUMEXCEPTION_H

#include <exception>
#include <string>


class AccumException: public std::exception
{
	public:
		enum AccumExc:int{	UNDEFINED				= 0,
							NO_ERR					= 1,
							CLOSE_SERV				= 2,
							DISCONNECT				= 3,
							DEFAULT_EXC				= 100,
							SRV_CREAT_SOCK_EXC		= 101,
							SRV_BIND_PORT_EXC		= 102,
							SRV_BIND_IP_EXC			= 103,
							SRV_LIST_EXC			= 104,
							SRV_ACCEPT_EXC			= 105,
							INV_CL_NUM_EXC			= 201,
							INV_IP_STR_EXC			= 202,
							INV_JSON_EXC			= 203,
							FILE_NEXS_EXC			= 304,
							CREAT_CHILD_ERR			= 401,
							NO_WPROGRAM_ERR			= 402,
							START_WPROG_ERR			= 403,
							CREAT_PIPE_ERR			= 404,
							OPEN_IPIPE_ERR			= 405,
							WRITE_PIPE_ERR			= 407,
							READ_PIPE_ERR			= 408,
							SEND_SOCK_ERR			= 409,
							OPEN_OPIPE_ERR			= 410,
							SELECT_ERR				= 411
		};
		AccumException(AccumExc exc);
		virtual const char* what() const noexcept;
		~AccumException();
	private:
		std::string getExcMessage(AccumExc exc) const;
		std::string error;
};

#endif
