#include "get_random.h"
#include <fstream>

int get_random(){
	std::ifstream r;
	r.open("/dev/random", std::ios::in);
	int dat;
	r >> dat;
	return dat;
}
