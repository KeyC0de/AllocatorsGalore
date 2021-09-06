#include <iostream>
#include <string>


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
	constexpr const std::size_t n = sizeof std::string;
	auto* p = alloc.allocate( n );
	alloc.construct( p,
		"Whatever" );
	std::cout << *p << '\n';
	alloc.destroy( p );
	alloc.deallocate( p,
		n );

	std::system( "pause" );
	return EXIT_SUCCESS;
}
