#include <iostream>
#include "ryu.h"

int main() {
	std::cout << "Lets give it a try...\n"
		<< "4.0      -> " << f2s(4.0f) << '\n'
		<< "4.1      -> " << f2s(4.1f) << '\n'
		<< "PI (f)   -> " << f2s(3.1415926535897932384626433f) << '\n'
		<< "PI (d)   -> " << d2s(3.1415926535897932384626433) << '\n'
		<< "2.1      -> " << f2s(2.1f) << '\n';
	return 0;
}
