#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include "minimal_allocator_cpp11.h"


int main()
{
	[[maybe_unused]]
	Allocator<int> a;
	Allocator<int> b;
	Allocator<std::string> c;
	std::cout << std::boolalpha << '\n';

	if ( a == b )
	{
		std::cout << "a == b" << '\n';
	}
	if ( a != c )
	{
		std::cout << "a != c" << '\n';
	}

	std::vector<std::string, Allocator<std::string>> v;
	v.reserve( 100 );
	v.emplace_back( "Hello" );
	v.emplace_back( "w/e" );
	v.emplace_back( "whatever" );
	v.emplace_back( "there is ist sofi j" );
	v.emplace_back( "wisdom" );
	v.emplace_back( "fear" );
	v.emplace_back( "there's more than meets the eye" );

	for ( const auto &s : v )
	{
		std::cout << s << '\n';
	}
	std::cout << typeid( v.get_allocator() ).name() << '\n';

	std::deque<int, Allocator<int>> dq;
	dq.push_back( 23 );
	dq.push_back( 90 );
	dq.push_back( 38794 );
	dq.push_back( 7 );
	dq.push_back( 0 );
	dq.push_back( 2 );
	dq.push_back( 13 );
	dq.push_back( 24323 );
	dq.push_back( 0 );
	dq.push_back( 1234 );
	for ( const auto &i : dq )
	{
		std::cout << i << '\n';
	}
	std::cout << typeid( dq.get_allocator() ).name() << '\n';

	std::cout << ( dq.get_allocator() == v.get_allocator() ) << '\n';

	std::system( "pause" );
	return 0;
}