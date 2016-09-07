#include "accumexception.h"

AccumException::AccumException(AccumExc exc)
{
	this->exc = exc;
}

const char* AccumException::what() const noexcept
{
	return getExcMessage(exc).c_str();
}

std::string AccumException::getExcMessage(AccumExc exc) const
{
	switch (exc)
	{
		case SRV_CREATE_EXC:
			return "Exception while creating server-socket descriptor";
	}

	return "";
}

AccumException::~AccumException()
{
}
