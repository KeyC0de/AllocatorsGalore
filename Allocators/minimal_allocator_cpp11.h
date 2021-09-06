#pragma once

#include "../allocator_utils.h"


//=============================================================
//	\class	Allocator
//
//	\author	KeyC0de
//	\date	2021/09/06 14:31
//
//	\brief	minimal stateless allocator C++11 compatible
//=============================================================
template <class T>
struct Allocator
{
	using value_type = T;

	Allocator()
	{

	}

	template<class U>
	Allocator( const Allocator<U>& ) noexcept
	{

	}

	// allocates memory equal to n objects of type T
	[[nodiscard]]
	value_type* allocate( std::size_t n )
	{
		if ( n > std::size_t(-1) / sizeof( T ) ) // if (n > max_size())
		{
			throw std::bad_alloc{};
		}
		if ( T* p = static_cast<T*>( ::operator new( sizeof( T ) * n ) ) )
		{
			return p;
		}
		throw std::bad_alloc{};
	}

	void deallocate( value_type* p,
		[[maybe_unused]] std::size_t count ) noexcept
	{
		::operator delete( p );
	}

	Allocator select_on_container_copy_construction() const noexcept
	{
		return *this;
	}
};

template<class T, class U>
bool operator==( const Allocator<T>&,
	const Allocator<U>& ) noexcept
{
	return true;
}

template<class T, class U>
bool operator!=( const Allocator<T>&,
	const Allocator<U>& ) noexcept
{
	return false;
}