#include <iostream>

constexpr const std::size_t n = 10;


int main()
{
	std::allocator<std::string> alloc;
	std::allocator<std::string> al;
	std::allocator<int> ali;
	std::allocator<double> ald;

	std::cout << std::boolalpha << '\n';
	std::cout << (alloc == al) << '\n';
	std::cout << (alloc != al) << '\n';
	std::cout << (alloc == ali) << '\n';
	std::cout << (ali == ald) << '\n';
	std::cout << (ali != ald) << '\n';


	auto* p = alloc.allocate(6);
	alloc.construct(p, "Whatever");
	std::cout << *p << '\n';
	alloc.destroy(p);
	alloc.deallocate(p, n);
	return 0;
}