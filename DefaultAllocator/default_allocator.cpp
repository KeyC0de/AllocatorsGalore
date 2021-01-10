#include <iostream>
#include <string>


constexpr const std::size_t n = 10;

int main()
{
	std::allocator<std::string> alloc;
	std::allocator<std::string> al;
	std::allocator<int> ali;
	std::allocator<double> ald;

	std::cout << std::boolalpha << '\n';
	std::cout << ( alloc == al ) << '\n';
	std::cout << ( alloc != al ) << '\n';
	std::cout << ( alloc == ali ) << '\n';
	std::cout << ( ali == ald ) << '\n';
	std::cout << ( ali != ald ) << '\n';

	// using an allocator to do:
	// 1. allocation (::operator new or malloc)
	// 2. construction (placement new)
	// 3. destruction (::operator delete or free)
	// 4. deallocation (placement delete)
	auto* p = alloc.allocate(6);
	alloc.construct( p, "Whatever" );
	std::cout << *p << '\n';
	alloc.destroy( p );
	alloc.deallocate( p, n );

	std::system( "pause" );
	return 0;
}