#ifndef GLOBALS_H
#define GLOBALS_H

#include <iostream>

#ifdef DEBUG
	#define dprint(x) do {std::cerr << x << std::endl;}while(0)
#else
	#define dprint(x) do {}while(0)
#endif

#endif
