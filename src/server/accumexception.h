#ifndef ACCUMEXCEPTION_H
#define ACCUMEXCEPTION_H

#include <exception>
#include <string>

class AccumException: public std::exception
{
	public:
		enum AccumExc:int{	SRV_CREATE_EXC		= 101,
							SRV_BIND_EXC		= 102
		};
		AccumException(AccumExc exc);
		virtual const char* what() const noexcept;
		~AccumException();
	private:
		std::string getExcMessage(AccumExc exc) const;
		AccumExc exc;
};

#endif
