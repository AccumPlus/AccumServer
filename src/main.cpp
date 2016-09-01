#include <iostream>
#include <class/class.h>

#ifdef DEBUG
	#define dprint(x) do {std::cerr << x << std::endl;}while(0)
#else
	#define dprint(x) do {}while(0)
#endif

int main()
{
	dprint("Ajores!");

	std::cout << VERSION << std::endl;

	Class c;
	c.print();

	/*const short N = 1000;
	const short M = 200;
	// Числа от 0 до 10000
	
	short mas[N];

	for (short i = 0; i < N; ++i)
		mas[i] = i;
	
	int a, b;

	for (short i = 0; i < M; ++i)
	{
		std::cin >> a >> b;
		if (mas[a] == mas[b]) continue;
		
		short k = mas[b];

		for (short j = 0; j < N; ++j)
			if (mas[j] == k)
				mas[j] = mas[a];
	}

	for (short i = 0; i < N; ++i)
		std::cout << mas[i] << std::endl;*/

	return 0;
}

