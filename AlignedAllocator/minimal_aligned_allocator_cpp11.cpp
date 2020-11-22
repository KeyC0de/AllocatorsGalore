#include <iostream>
#include "minimal_aligned_allocator_cpp11.h"

int main()
{
	[[maybe_unused]]
	AlignedAllocator<int> a;
	AlignedAllocator<int> b;
	AlignedAllocator<std::string> c;
	std::cout << std::boolalpha << '\n';

	if (a == b)
		std::cout << "a == b" << '\n';
	if (a != c)
		std::cout << "a != c" << '\n';

	std::vector<std::string, AlignedAllocator<std::string>> v;
	v.reserve(100);
	v.push_back("Hello");
	v.push_back("w/e");
	v.push_back("whatever");
	v.push_back("there is ist sofi j");
	v.push_back("wisdom");
	v.push_back("fear");
	v.push_back("there's more than meets the eye");

	for (const auto &s : v)
	{
		std::cout << s << '\n';
	}
	std::cout << typeid(v.get_allocator()).name() << '\n';

	std::deque<int, AlignedAllocator<int>> dq;
	dq.push_back(23);
	dq.push_back(90);
	dq.push_back(38794);
	dq.push_back(7);
	dq.push_back(0);
	dq.push_back(2);
	dq.push_back(13);
	dq.push_back(24323);
	dq.push_back(0);
	dq.push_back(1234);
	for (const auto &i : dq)
	{
		std::cout << i << '\n';
	}
	std::cout << typeid(dq.get_allocator()).name() << '\n';

	std::cout << (dq.get_allocator() == v.get_allocator()) << '\n';
}